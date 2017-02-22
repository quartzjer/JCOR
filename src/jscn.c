#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "jscn.h"
#include "js0n.h"
#include "base64.h"
#include "base16.h"

// working state
typedef struct jscn_s {
  uint8_t *start;
  uint8_t *out;
  cb0r_t dict;
} jscn_s, *jscn_t;

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

static bool on2cn_part(jscn_t state, uint8_t *in, size_t inlen, bool iskey)
{
  if(in[0] == '{' || in[0] == '[')
  {
    // count items, write cbor map/array+count, recurse each k/v
    uint16_t i=0;
    size_t len = 0;
    for(;js0n(NULL,i,(char*)in,inlen,&len,NULL);i++);
    if(in[0] == '{')
    {
      state->out += ctype(state->out, CB0R_MAP, i / 2);
    } else {
      state->out += ctype(state->out, CB0R_ARRAY, i);
    }
    for(uint16_t j=0;j<i;j++)
    {
      const char *str = js0n(NULL,j,(char*)in,inlen,&len,NULL);
      on2cn_part(state, (uint8_t *)str, len, (in[0] == '{' && (j & 1) == 0));
    }

  }else if(in[inlen] == '"'){ // js0n type detection pattern :/
    uint32_t blen = 0;
    uint8_t *start = state->out;
    if(!iskey && inlen > 10 && (blen = base64_decoder((char*)in,inlen,NULL)))
    {
      // if is base64 write cbor tag and byte string
      state->out += ctype(state->out, CB0R_TAG, 21);
      state->out += ctype(state->out, CB0R_BYTE, blen);
      base64_decoder((char *)in, inlen, state->out);
      // validate exact match using out as buffer
      base64_encoder(state->out, blen, (char *)(state->out + blen));
      if (memcmp(state->out + blen, in, inlen) == 0)
      {
        state->out += blen;
      }else{
        blen = 0;
      }
    }
    if(!iskey && base16_check((char*)in,inlen))
    {
      blen = inlen/2;
      state->out += ctype(state->out, CB0R_TAG, 23);
      state->out += ctype(state->out, CB0R_BYTE, blen);
      base16_decode((char *)in, inlen, state->out);
      state->out += blen;
    }
    // write cbor UTF8
    if(blen == 0)
    {
      state->out += ctype(state->out, CB0R_UTF8, inlen);
      memcpy(state->out, in, inlen);
      state->out += inlen;
    }
    // check dictionary
    cb0r_s item = {0,};
    cb0r(start,state->out,0,&item);
    int32_t index = jscn_geti(state->dict, &item);
    if(index > 0 && index < 256)
    {
      state->out = start;
      // cbor byte key to represent value
      state->out += ctype(state->out, CB0R_BYTE, 1);
      state->out[0] = index;
      state->out++;
    }
  }else if(memcmp(in,"false",inlen) == 0){
    state->out += ctype(state->out, CB0R_SIMPLE, 20);
  }else if(memcmp(in,"true",inlen) == 0){
    state->out += ctype(state->out, CB0R_SIMPLE, 21);
  }else if(memcmp(in,"null",inlen) == 0){
    state->out += ctype(state->out, CB0R_SIMPLE, 22);
  }else if(memchr(in,'.',inlen) || memchr(in,'e',inlen) || memchr(in,'E',inlen)){
    // TODO write cbor tag 4 for decimal floats/exponents
  }else{
    // parse number, write cbor int
    long long num = strtoll((const char*)in,NULL,10);
    if(num < 0)
      state->out += ctype(state->out, CB0R_NEG, (uint64_t)(~num));
    else
      state->out += ctype(state->out, CB0R_INT, (uint64_t)num);
  }

  return true;
}

size_t json2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict)
{
  if(!in || !inlen) return 0;

  // validate any json first while getting whitespace
  uint8_t **whitespace = malloc((inlen + 1) * sizeof(char *)); // array of char*'s
  memset(whitespace, 0, (inlen + 1) * sizeof(char *));
  size_t err = 0;
  js0n("\0", 1, (char *)in, inlen, &err, (char**)whitespace); // full scan by looking for invalid key
  for(uint8_t **ws = whitespace;*ws;ws++) printf("%lu:%u ",*ws - in, **ws);
  printf("\n");
  free(whitespace);
  if(err) return 0;

  // first make the JSCN header map
  uint8_t *at = out;
  at += ctype(at, CB0R_TAG, 42);
  uint8_t items = 1;
  if(dict) items++;
  if(whitespace) items++;
  at += ctype(at, CB0R_MAP, items * 2);

  if(dict)
  {
    at += ctype(at, CB0R_INT, 2);
    cb0r_s res = {0,};
    jscn_getv(dict, 0, &res);
    memcpy(at,res.start,res.end-res.start);
    at += res.end-res.start;
  }

  // generate into value
  at += ctype(at, CB0R_INT, 1);
  jscn_s state = {0,};
  state.start = at;
  state.out = at;
  state.dict = dict;
  on2cn_part(&state, in, inlen, false);
  at = state.out;

  // add any whitespace
  if(whitespace)
  {
    at += ctype(at, CB0R_INT, 3);
    at += ctype(at, CB0R_MAP, 0);
    // TODO
  }

  return at - out;
}

size_t jscn2on(uint8_t *in, size_t inlen, char *out, uint32_t skip, cb0r_t dict)
{
  size_t outlen = 0;
  cb0r_s res = {0,};
  uint8_t *end = cb0r(in,in+inlen,skip,&res);

  // dictionary replacement swap
  if(res.type == CB0R_BYTE && res.length == 1 && !jscn_getv(dict,res.start[0],&res))
  {
    printf("not found in dictionary: %u\n",res.start[0]);
    return 0;
  }

  // expand by type
  switch(res.type)
  {
    case CB0R_INT: {
      outlen = sprintf(out,"%llu",res.value);
    } break;
    case CB0R_NEG: {
      outlen = sprintf(out,"-%llu",res.value+1);
    } break;
    case CB0R_UTF8: {
      outlen = sprintf(out,"\"%.*s\"",(int)res.length,res.start);
    } break;
    case CB0R_ARRAY:
    case CB0R_MAP: {
      outlen += sprintf(out+outlen,(res.type==CB0R_MAP)?"{":"[");
      for(uint32_t i=0;i<res.count;i++)
      {
        if(i) outlen += sprintf(out+outlen,",");
        outlen += jscn2on(res.start,end-res.start,out+outlen,i,dict);
        if(res.type != CB0R_MAP) continue;
        outlen += sprintf(out+outlen,":");
        outlen += jscn2on(res.start,end-res.start,out+outlen,++i,dict);
      }
      outlen += sprintf(out+outlen,(res.type==CB0R_MAP)?"}":"]");
    } break;
    case CB0R_BASE64URL: {
      cb0r_s res2 = {0,};
      cb0r(res.start,end,0,&res2);
      if(res2.type == CB0R_BYTE)
      {
        out[0] = '"';
        outlen = base64_encoder(res2.start,res2.length,out+1);
        out[outlen+1] = '"';
        outlen += 2;
      }
    } break;
    case CB0R_HEX: {
      cb0r_s res2 = {0,};
      cb0r(res.start,end,0,&res2);
      if(res2.type == CB0R_BYTE)
      {
        out[0] = '"';
        outlen = res2.length*2;
        base16_encode(res2.start,res2.length,out+1);
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

size_t jwt2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict)
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
  outlen += json2cn(buf, len, out+outlen, dict);

  len = base64_decoder(dot1, (dot2-dot1)-1, buf);
  outlen += json2cn(buf, (dot2-dot1)-1, out+outlen, dict);
  
  outlen += ctype(out+outlen,CB0R_TAG,21);
  len = base64_decoder(dot2, (end-dot2)+1, buf);
  outlen += ctype(out+outlen,CB0R_BYTE,len);
  memcpy(out+outlen,buf,len);
  outlen += len;

  free(buf);
  return outlen;
}

// fetch value at given index of cbor array
bool jscn_getv(cb0r_t array, uint32_t index, cb0r_t val)
{
  cb0r_s res = {0,};
  uint8_t *end = cb0r(array->start,array->start+array->length,0,&res);
  if(res.type != CB0R_ARRAY) return false;
  if(!index || index >= res.count) return false;
  if(!cb0r(res.start,end,index,val)) return false;
  return true;
}


// match raw cbor value in array and return index (-1 if none)
int32_t jscn_geti(cb0r_t array, cb0r_t item)
{
  if(!array || !item) return -1;
  uint32_t vlen = item->end - item->start;
  if(!vlen) return -1;
  cb0r_s res = {0,};
  if(!cb0r(array->start,array->start+array->length,0,&res)) return -1;
  if(res.type != CB0R_ARRAY) return -1;
  for(uint32_t i=0;i<res.count;i++)
  {
    cb0r_s res2 = {0,};
    if(!cb0r(res.start, res.end, i, &res2)) break;
    if(res2.type == item->type && res2.value == item->value && memcmp(res2.start,item->start,vlen) == 0) return i;
  }
  return -1;
}

// app defines these to resolve a dictionary by id or by json object
__weak cb0r_t jscn_dict_id(int32_t id)
{
  return NULL;
}

__weak cb0r_t jscn_dict_match(uint8_t *obj, size_t objlen)
{
  return NULL;
}

