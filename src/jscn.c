#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "jscn.h"
#include "js0n.h"
#include "base64.h"
#include "base16.h"

// whitespace table
static char *ws_table[24] = { "0a", "0a2020", "0a20202020", "0a202020202020", "0a2020202020202020", "0a20202020202020202020", "0a202020202020202020202020", "0a2020202020202020202020202020", "09", "0a09", "0a0909", "0a090909", "0a09090909", "0a0909090909", "0a090909090909", "0a09090909090909", "0a0909090909090909", "0d", "0d0a", "0d0a2020", "0d0a20202020", "0d0a09", "0d0a0909", "0d0a090909"};
static uint8_t ws_tablen[24] = { 2, 6, 10, 14, 18, 22, 26, 30, 2, 4, 6, 8, 10, 12, 14, 16, 18, 2, 4, 8, 12, 6, 8, 10};
#define WS_MAXLEN 32

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

bool cbeq(cb0r_t a, cb0r_t b)
{
  if(!a || !b) return false;
  if(a->type != b->type) return false;
  if(a->value != b->value) return false;
  switch(a->type)
  {
    case CB0R_INT:
    case CB0R_NEG:
    case CB0R_TAG:
    case CB0R_SIMPLE:
      return true;
    case CB0R_UTF8:
    case CB0R_BYTE:
    case CB0R_MAP:
    case CB0R_ARRAY:
      if(!a->start || !b->start) return false;
      if(!a->end || !b->end) return false;
      if((a->end - a->start) != (b->end - b->start)) return false;
      if(memcmp(a->start,b->start,(a->end - a->start)) != 0) return false;
      return true;
    default:
      return false;
  }
  return false;
}

static void on2cn_part(jscn_t state, uint8_t *in, size_t inlen, bool iskey)
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
}

static void ws2cn(jscn_t state, uint8_t *in, size_t inlen)
{
  // get all whitespace pointers
  char **whitespace = malloc((inlen + 1) * sizeof(char *)); // temporary array of char*'s
  memset(whitespace, 0, (inlen + 1) * sizeof(char *)); // js0n fills only nulls
  whitespace[inlen] = "end";
  size_t err = 0;
  js0n("\0", 1, (char *)in, inlen, &err, whitespace);
  whitespace[inlen] = NULL; // ensure terminated

  // loop to count first, skip consecutive
  uint32_t items = 0;
  for(char **i = whitespace; *i; items++)
    for(char **j = i++; *i && (*i - *j) == 1; i++, j++);
  state->out += ctype(state->out, CB0R_INT, 3);
  // indefinite length array
  state->out[0] = CB0R_ARRAY << 5;
  state->out[0] |= 31;
  state->out++;

  // fill in array w/ integers
  uint32_t prev = 0;
  for(char **i = whitespace; *i;) {
    // start/end are _inclusive_
    char **start = i, **end = i;
    for(i++; *i && (*i - *end) == 1; i++, end++);
    do {
      uint8_t *ws = (uint8_t*)*start;
      uint32_t wslen = (*end - *start) + 1;
      uint32_t offset = (ws - in) - prev;
      prev = ws - in;
//      printf("WS off %u %.*s\n", offset, wslen * 2, base16_encode(ws, wslen, (char *)state->out));

      // single space special case first
      if(wslen == 1 && ws[0] == ' ')
      {
        state->out += ctype(state->out, CB0R_NEG, offset);
        break;
      }

      // then add offset
      state->out += ctype(state->out, CB0R_INT, offset);

      // count any repeating/leading spaces
      uint32_t spaces = 0;
      for(uint32_t j=0;j<wslen;j++) if(ws[j] == ' ') spaces++; else break;
      if(spaces) {
        state->out += ctype(state->out, CB0R_NEG, spaces);
        *start += spaces;
        continue;
      }

      // look for the longest prefix from the table
      char wsmatch[WS_MAXLEN] = {0,};
      base16_encode(ws, (wslen > WS_MAXLEN) ? WS_MAXLEN : wslen, wsmatch);
      int8_t best = -1;
      for(uint8_t j=0;j<=23;j++) {
        // the current ws must match 
        if(ws_tablen[j] > (wslen*2)) continue;
        if(memcmp(wsmatch, ws_table[j], ws_tablen[j]) != 0) continue;
        best = j;
      }
      if(best == -1) printf("FATAL ERROR: %s\n",wsmatch);

      // save table id and advance
      state->out += ctype(state->out, CB0R_INT, best);
      *start += ws_tablen[best];

    } while(*start < *end);
  }

  free(whitespace);
}

size_t json2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict)
{
  if(!in || !inlen) return 0;

  // validate any json first w/ full scan by looking for invalid key
  size_t err = 0;
  char *ws[2] = {NULL,"end"}; // notice if there's any whitespace also
  js0n("\0", 1, (char *)in, inlen, &err, ws);
  if(err) return 0;

  // first make the JSCN header map
  uint8_t *at = out;
  at += ctype(at, CB0R_TAG, 42);
  uint8_t items = 1;
  if(dict) items++;
  if(ws[0]) items++;
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

  // add any whitespace
  if(ws[0]) ws2cn(&state, in, inlen);

  return state.out - out;
}

static size_t cn2on_part(uint8_t *in, size_t inlen, char *out, uint32_t skip, cb0r_t dict)
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
        outlen += cn2on_part(res.start,end-res.start,out+outlen,i,dict);
        if(res.type != CB0R_MAP) continue;
        outlen += sprintf(out+outlen,":");
        outlen += cn2on_part(res.start,end-res.start,out+outlen,++i,dict);
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

cb0r_t jscn2cbor(uint8_t *in, size_t inlen, cb0r_t out, uint8_t index)
{
  cb0r_s res = {0,};

  uint8_t *end = cb0r(in,in+inlen,0,&res);
  if(res.type != CB0R_TAG || res.value != 42)
  {
    printf("CBOR is not tagged as JSCN\n");
    return NULL;
  }

  end = cb0r(in,in+inlen,1,&res);
  if(res.type != CB0R_MAP || !res.count)
  {
    printf("JSCN does not begin with a map\n");
    return NULL;
  }

  cb0r_s resv = {0,};
  if(!jscn_getkv(&res,&(cb0r_s){.type=CB0R_INT,.value=index},&resv))
  {
    printf("map does not contain key %u\n",index);
    return NULL;
  }

  return out;
}

size_t jscn2on(uint8_t *in, size_t inlen, char *out, cb0r_t dict)
{
  size_t outlen = 0;
  cb0r_s res = {0,};
  jscn2cbor(in, inlen, &res, 1);
  outlen = cn2on_part(res.start, (res.end - res.start), out, 0, dict);
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
    if(cbeq(item,&res2)) return i;
  }
  return -1;
}

bool jscn_getkv(cb0r_t map, cb0r_t key, cb0r_t value)
{
  return false;
}
