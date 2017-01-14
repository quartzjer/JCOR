#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cjs.h"
#include "js0n.h"
#include "base64.h"

// copied from https://gist.github.com/yinyin/2027912
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif	/* __APPLE__ */

size_t ctype(uint8_t *out, cb0r_e type, uint64_t value)
{
  out[0] = type << 5;
  if(value <= 23)
  {
    out[0] |= value;
    return 1;
  }
  if(value >= UINT32_MAX)
  {
    out[0] |= 27;
    value = htobe64(value);
    memcpy(out+1,&value,8);
    return 9;
  }
  if(value > UINT16_MAX)
  {
    out[0] |= 26;
    value = htobe32(value);
    memcpy(out+1,&value,4);
    return 5;
  }
  if(value >= UINT8_MAX)
  {
    out[0] |= 25;
    value = htobe16(value);
    memcpy(out+1,&value,2);
    return 3;
  }
  out[0] |= 24;
  out[1] = value;
  return 2;
}

size_t js2cb(uint8_t *in, size_t inlen, uint8_t *out, bool iskey, cb0r_t dict)
{
  size_t outlen = 0;

  if(in[0] == '{' || in[0] == '[')
  {
    // count items, write cbor map/array+count, recurse each k/v
    uint16_t i=0;
    for(;js0n(NULL,i,(char*)in,inlen,&outlen);i++);
    if(in[0] == '{')
    {
      outlen = ctype(out,CB0R_MAP,i/2);
      cb0r_t d2 = dict_match(in,inlen);
      if(d2) dict = d2;
    }else{
      outlen = ctype(out,CB0R_ARRAY,i);
    }
    for(uint16_t j=0;j<i;j++)
    {
      size_t len = 0;
      const char *str = js0n(NULL,j,(char*)in,inlen,&len);
      outlen += js2cb((uint8_t*)str,len,out+outlen,(in[0] == '{' && (j & 1) == 0), dict);
    }

  }else if(in[inlen] == '"'){ // js0n type detection pattern :/
    // check if is base64
    uint32_t b64 = 0;
    if(!iskey && (b64 = base64_decoder((char*)in,inlen,NULL)))
    {
      // write cbor tag and byte string
      outlen = ctype(out,CB0R_TAG,21);
      outlen += ctype(out+outlen,CB0R_BYTE,b64);
      base64_decoder((char*)in,inlen,out+outlen);
      // validate exact match using out as buffer
      base64_encoder(out+outlen,b64,(char*)(out+outlen+b64));
      if(memcmp(out+outlen+b64,in,inlen) == 0)
      {
        outlen += b64;
      }else{
        b64 = 0;
      }
    }
    if(b64 == 0)
    {
      // check dictionary
      int32_t index = cb_geti(dict,in,inlen);
      if(index > 0 && index < 256)
      {
        // cbor byte key to represent value
        outlen = ctype(out,CB0R_BYTE,1);
        out[outlen] = index;
        outlen++;
      }else{
        // write cbor UTF8
        outlen = ctype(out,CB0R_UTF8,inlen);
        memcpy(out+outlen,in,inlen);
        outlen += inlen;
      }
    }
  }else if(memcmp(in,"false",inlen) == 0){
    outlen = ctype(out,CB0R_SIMPLE,CB0R_FALSE);
  }else if(memcmp(in,"true",inlen) == 0){
    outlen = ctype(out,CB0R_SIMPLE,CB0R_TRUE);
  }else if(memcmp(in,"null",inlen) == 0){
    outlen = ctype(out,CB0R_SIMPLE,CB0R_NULL);
  }else if(memchr(in,'.',inlen) || memchr(in,'e',inlen) || memchr(in,'E',inlen)){
    // TODO write cbor tag 4 for decimal floats/exponents
  }else{
    // parse number, write cbor int
    long long num = strtoll((const char*)in,NULL,10);
    if(num < 0) outlen = ctype(out,CB0R_NEG,(uint64_t)(~num));
    else outlen = ctype(out,CB0R_INT,(uint64_t)num);
  }

  return outlen;
}

size_t cb2js(uint8_t *in, size_t inlen, char *out, uint32_t skip, cb0r_t dict)
{
  size_t outlen = 0;
  cb0r_s res = {0,};
  uint8_t *end = cb0r(in,in+inlen,skip,&res);
  switch(res.type)
  {
    case CB0R_INT: {
      outlen = sprintf(out,"%llu",res.value);
    } break;
    case CB0R_NEG: {
      outlen = sprintf(out,"-%llu",res.value+1);
    } break;
    case CB0R_BYTE: {
      // dictionary expansion
      cb0r_s str = {0,};
      if(res.length != 1 || !cb_getv(dict,res.start[0],&str))
      {
        printf("not found in dictionary: %u\n",res.start[0]);
        break;
      }
      memcpy(&res,&str,sizeof(cb0r_s));
    } // fall through!
    case CB0R_UTF8: {
      outlen = sprintf(out,"\"%.*s\"",(int)res.length,res.start);
    } break;
    case CB0R_ARRAY: {
      outlen += sprintf(out+outlen,"[");
      for(uint32_t i=0;i<res.count;i++)
      {
        if(i) outlen += sprintf(out+outlen,",");
        outlen += cb2js(res.start,end-res.start,out+outlen,i,dict);
      }
      outlen += sprintf(out+outlen,"]");
    } break;
    case CB0R_MAP: {
      outlen += sprintf(out+outlen,"{");
      for(uint32_t i=0;i<res.count;i++)
      {
        if(i) outlen += sprintf(out+outlen,",");
        outlen += cb2js(res.start,end-res.start,out+outlen,i*2,dict);
        outlen += sprintf(out+outlen,":");
        outlen += cb2js(res.start,end-res.start,out+outlen,(i*2)+1,dict);
      }
      outlen += sprintf(out+outlen,"}");
    } break;
    case CB0R_TAG: {
      cb0r_s res2 = {0,};
      cb0r(res.start,end,0,&res2);
      if(res.value == 21 && res2.type == CB0R_BYTE)
      {
        out[0] = '"';
        outlen = base64_encoder(res2.start,res2.length,out+1);
        out[outlen+1] = '"';
        outlen += 2;
      }
    } break;
    case CB0R_FALSE: {
      outlen = sprintf(out,"false");
    } break;
    case CB0R_TRUE: {
      outlen = sprintf(out,"true");
    } break;
    case CB0R_NULL: {
      outlen = sprintf(out,"null");
    } break;
    default: {
      printf("unsupported type: %u\n",res.type);
    } break;

  }
  return outlen;
}

size_t jwt2cb(uint8_t *in, size_t inlen, uint8_t *out)
{
  char *encoded = (char*)in;
  char *end = encoded+(inlen-1);
  
  // make sure the dot separators are there
  char *dot1 = memchr(encoded,'.',end-encoded);
  if(!dot1 || dot1+1 >= end) return 0;
  dot1++;
  char *dot2 = memchr(dot1,'.',end-dot1);
  if(!dot2 || (dot2+1) >= end) return 0;
  dot2++;

  uint8_t *buf = malloc(inlen); // scratch space
  size_t outlen = ctype(out,CB0R_ARRAY,3);

  size_t len = base64_decoder(encoded, (dot1-encoded)-1, buf);
  outlen += js2cb(buf, len, out+outlen, false, NULL);

  len = base64_decoder(dot1, (dot2-dot1)-1, buf);
  outlen += js2cb(buf, (dot2-dot1)-1, out+outlen, false, NULL);
  
  outlen += ctype(out+outlen,CB0R_TAG,21);
  len = base64_decoder(dot2, (end-dot2)+1, buf);
  outlen += ctype(out+outlen,CB0R_BYTE,len);
  memcpy(out+outlen,buf,len);
  outlen += len;

  free(buf);
  return outlen;
}

// fetch utf8 string value at given index of cbor array
bool cb_getv(cb0r_t array, uint32_t index, cb0r_t val)
{
  cb0r_s res = {0,};
  uint8_t *end = cb0r(array->start,array->start+array->length,0,&res);
  if(res.type != CB0R_ARRAY) return false;
  if(index >= res.count) return false;
  end = cb0r(res.start,end,index,&res);
  if(res.type != CB0R_UTF8) return false;
  memcpy(val,&res,sizeof(cb0r_s));
  return true;
}


// match string value in array and return index (-1 if none)
int32_t cb_geti(cb0r_t array, uint8_t *str, uint32_t len)
{
  if(!array || !str) return -1;
  cb0r_s res = {0,};
  uint8_t *end = cb0r(array->start,array->start+array->length,0,&res);
  if(res.type != CB0R_ARRAY) return -1;
  for(uint32_t i=0;i<res.count;i++)
  {
    cb0r_s ires = {0,};
    cb0r(res.start,end,i,&ires);
    if(ires.type != CB0R_UTF8) continue;
    if(ires.length != len) continue;
    if(memcmp(str,ires.start,len) != 0) continue;
    return i;
  }
  return -1;
}

// app defines these to resolve a dictionary by id or by json object
__weak cb0r_t dict_id(int32_t id)
{
  return NULL;
}

__weak cb0r_t dict_match(uint8_t *obj, size_t objlen)
{
  return NULL;
}

