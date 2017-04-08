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

// internal working state
typedef struct state_s {
  uint8_t *start;
  uint8_t *out;
  jscn_t dict;
} state_s, *state_t;

// named keys for JSCN map
enum {
  JSCN_KEY_DATA = 1,
  JSCN_KEY_DICT,
  JSCN_KEY_WS
};

// match raw cbor value in array and return index (-1 if none)
static int32_t get_index(cb0r_t array, uint8_t *str, uint32_t len)
{
  if(!array || !str || !len) return -1;
  for(uint32_t i = 0; i < array->count; i++) {
    cb0r_s res = {0,};
    if(!cb0r(array->start + array->header, array->end, i, &res)) break;
    if((res.end - (res.start + res.header)) != len) continue;
    if(memcmp(str, res.start + res.header, len) == 0) return i;
  }
  return -1;
}

static void on2cn_part(state_t state, uint8_t *in, size_t inlen, bool iskey)
{
  if(in[0] == '{' || in[0] == '[')
  {
    // count items, write cbor map/array+count, recurse each k/v
    uint16_t i=0;
    size_t len = 0;
    for(;js0n(NULL,i,(char*)in,inlen,&len,NULL);i++);
    if(in[0] == '{')
    {
      state->out += cb0r_write(state->out, CB0R_MAP, i / 2);
    } else {
      state->out += cb0r_write(state->out, CB0R_ARRAY, i);
    }
    for(uint16_t j=0;j<i;j++)
    {
      const char *str = js0n(NULL,j,(char*)in,inlen,&len,NULL);
      on2cn_part(state, (uint8_t *)str, len, (in[0] == '{' && (j & 1) == 0));
    }

  }else if(in[inlen] == '"'){ // js0n type detection pattern :/

    // base64 value check
    uint32_t blen = 0;
    uint8_t *buff = state->out + 32; // temporary buffer padded out to leave room for writing tags
    if(!iskey && inlen > 10 && (blen = base64_decoder((char *)in, inlen, buff))) {
      // validate exact match using out as buffer
      base64_encoder(buff, blen, (char *)(buff + blen));
      if(memcmp(buff + blen, in, inlen) == 0) {
        state->out += cb0r_write(state->out, CB0R_TAG, 21);

        // check if the decoded is itself JSON
        uint32_t blen2 = jscn_parse(buff, blen, buff + blen, state->dict);
        if(blen2 && blen2 < blen){
          printf("recursed %u < %u: %.*s\n",blen2,blen,blen,(char*)buff);
          memmove(state->out, buff + blen, blen2);
          state->out += blen2;
          return;
        }

        // if just base64 insert decoded byte string
        state->out += cb0r_write(state->out, CB0R_BYTE, blen);
        memmove(state->out, buff, blen);
        state->out += blen;
        return;
      }
      // not b64, fall through
    }

    // base16 value check
    if(!iskey && base16_check((char*)in,inlen))
    {
      blen = inlen/2;
      state->out += cb0r_write(state->out, CB0R_TAG, 23);
      state->out += cb0r_write(state->out, CB0R_BYTE, blen);
      base16_decode((char *)in, inlen, state->out);
      state->out += blen;
      return;
    }

    // check dictionary for UTF8 swap
    int32_t index;
    if(state->dict && (index = get_index(&(state->dict->data), in, inlen)) && index > 0 && index < 256) {
      // cbor byte key to represent value
      state->out += cb0r_write(state->out, CB0R_BYTE, 1);
      state->out[0] = index;
      state->out++;
    }else{
      // write CBOR UTF8
      state->out += cb0r_write(state->out, CB0R_UTF8, inlen);
      memcpy(state->out, in, inlen);
      state->out += inlen;
    }

  }else if(memcmp(in,"false",inlen) == 0){
    state->out += cb0r_write(state->out, CB0R_SIMPLE, 20);
  }else if(memcmp(in,"true",inlen) == 0){
    state->out += cb0r_write(state->out, CB0R_SIMPLE, 21);
  }else if(memcmp(in,"null",inlen) == 0){
    state->out += cb0r_write(state->out, CB0R_SIMPLE, 22);
  }else if(memchr(in,'.',inlen) || memchr(in,'e',inlen) || memchr(in,'E',inlen)){
    // TODO write cbor tag 4 for decimal floats/exponents
  }else{
    // parse number, write cbor int
    long long num = strtoll((const char*)in,NULL,10);
    if(num < 0)
      state->out += cb0r_write(state->out, CB0R_NEG, (uint64_t)(~num));
    else
      state->out += cb0r_write(state->out, CB0R_INT, (uint64_t)num);
  }
}

static void ws2cn(state_t state, uint8_t *in, size_t inlen)
{
  // get all whitespace pointers
  char **whitespace = malloc((inlen + 1) * sizeof(char *)); // temporary array of char*'s
  memset(whitespace, 0, (inlen + 1) * sizeof(char *)); // js0n fills only nulls
  whitespace[inlen] = "end";
  size_t err = 0;
  js0n("\0", 1, (char *)in, inlen, &err, whitespace);
  whitespace[inlen] = NULL; // ensure terminated

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
        state->out += cb0r_write(state->out, CB0R_NEG, offset);
        prev++;
        break;
      }

      // then add offset
      state->out += cb0r_write(state->out, CB0R_INT, offset);

      // count any repeating/leading spaces
      uint32_t spaces = 0;
      for(uint32_t j=0;j<wslen;j++) if(ws[j] == ' ') spaces++; else break;
      if(spaces) {
        state->out += cb0r_write(state->out, CB0R_NEG, spaces);
        *start += spaces;
        prev += spaces;
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
      state->out += cb0r_write(state->out, CB0R_INT, best);
      *start += ws_tablen[best]/2;
      prev += ws_tablen[best]/2;

    } while(*start < *end);
  }

  // add break to end of indefinite length array
  state->out[0] = 0xff;
  state->out++;

  free(whitespace);
}

size_t jscn_parse(uint8_t *in, size_t inlen, uint8_t *out, jscn_t dict)
{
  if(!in || !inlen) return 0;

  // validate any json first w/ full scan by looking for invalid key
  size_t err = 0;
  char *ws[2] = {NULL,"end"}; // notice if there's any whitespace also
  js0n("\0", 1, (char *)in, inlen, &err, ws);
  if(err) return 0;

  // first make the JSCN header map
  uint8_t *at = out;
  at += cb0r_write(at, CB0R_TAG, 42);
  uint8_t items = 1;
  if(dict) items++;
  if(ws[0]) items++;
  at += cb0r_write(at, CB0R_MAP, items);

  if(dict) {
    at += cb0r_write(at, CB0R_INT, JSCN_KEY_DICT);
    cb0r_s res = {0,};
    if(!cb0r_get(&(dict->data), 0, &res)) return 0;
    at += cb0r_write(at, CB0R_INT, res.value);
    printf("using dictionary %llu\n",res.value);
  }

  // generate into value
  at += cb0r_write(at, CB0R_INT, JSCN_KEY_DATA);
  state_s state = {0,};
  state.start = at;
  state.out = at;
  state.dict = dict;
  on2cn_part(&state, in, inlen, false);

  // add any whitespace
  if(ws[0]) {
    state.out += cb0r_write(state.out, CB0R_INT, JSCN_KEY_WS);
    ws2cn(&state, in, inlen);
  }

  return state.out - out;
}

static size_t cn2on_part(uint8_t *in, size_t inlen, char *out, uint32_t skip, jscn_t dict)
{
  size_t outlen = 0;
  cb0r_s res = {0,};
  uint8_t *end = cb0r(in,in+inlen,skip,&res);

  // dictionary replacement swap
  if(res.type == CB0R_BYTE && res.length == 1 && (!dict || !cb0r_get(&(dict->data), res.start[res.header], &res)))
  {
    printf("not found in dictionary: %u\n",res.start[res.header]);
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
      outlen = sprintf(out,"\"%.*s\"",(int)res.length,res.start+res.header);
    } break;
    case CB0R_ARRAY:
    case CB0R_MAP: {
      outlen += sprintf(out+outlen,(res.type==CB0R_MAP)?"{":"[");
      for(uint32_t i=0;i<res.count;i++)
      {
        if(i) outlen += sprintf(out+outlen,",");
        outlen += cn2on_part(res.start+res.header,end-(res.start+res.header),out+outlen,i,dict);
        if(res.type != CB0R_MAP) continue;
        outlen += sprintf(out+outlen,":");
        outlen += cn2on_part(res.start+res.header,end-(res.start+res.header),out+outlen,++i,dict);
      }
      outlen += sprintf(out+outlen,(res.type==CB0R_MAP)?"}":"]");
    } break;
    case CB0R_BASE64URL: {
      out[0] = '"';
      cb0r_s res2 = {0,};
      cb0r(res.start+res.header,end,0,&res2);
      jscn_s jscn = {0,};
      if(res2.type == CB0R_TAG && res2.value == 42 && jscn_load(res.start + res.header, res.end - (res.start + res.header), &jscn)) {
        jscn.dict = dict;
        printf("recursing len %lu\n", res.end - (res.start+res.header));
        outlen = jscn_stringify(&jscn, out+1, 0);
        outlen = base64_encoder((uint8_t *)(out + 1), outlen, out + 1);
      }else if(res2.type == CB0R_BYTE) {
        outlen = base64_encoder(res2.start+res.header, res2.length, out + 1);
      }
      out[outlen + 1] = '"';
      outlen += 2;
    } break;
    case CB0R_HEX: {
      cb0r_s res2 = {0,};
      cb0r(res.start+res.header,end,0,&res2);
      if(res2.type == CB0R_BYTE)
      {
        out[0] = '"';
        outlen = res2.length*2;
        base16_encode(res2.start+res.header,res2.length,out+1);
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

// validates jscn and returns the given index value from the wrapper
bool jscn_load(uint8_t *in, size_t inlen, jscn_t result)
{
  cb0r_s res = {0,};
  cb0r(in, in + inlen, 0, &res);
  if(res.type != CB0R_TAG || res.value != 42) {
    printf("CBOR is not tagged as JSCN: %u/%llu\n", res.type, res.value);
    return false;
  }

  cb0r(res.start + res.header, res.end, 0, &(result->jscn));
  if(result->jscn.type != CB0R_MAP || !result->jscn.count) {
    printf("JSCN does not begin with a map: %u\n", result->jscn.type);
    return false;
  }

  if(!cb0r_find(&(result->jscn), CB0R_INT, JSCN_KEY_DATA, NULL, &(result->data))) {
    printf("JSCN does not contain data");
    return false;
  }

  return true;
}

size_t jscn_stringify(jscn_t jscn, char *out, size_t len)
{
  // convert the raw data
  size_t outlen = cn2on_part(jscn->data.start, jscn->data.end - jscn->data.start, out, 0, jscn->dict);

  // check for whitespace hints
  cb0r_s res = {0,};
  if(cb0r_find(&(jscn->jscn), CB0R_INT, JSCN_KEY_WS, NULL, &res) && res.type == CB0R_ARRAY) {
    printf("using whitespace hints\n");
    cb0r_s item = {0,};
    uint32_t i = 0;
    char *at = out;
    while(item.type < CB0R_ERR) {
      cb0r(res.start + res.header, res.start + res.header + res.length, i++, &item);

      // insert single space first
      if(item.type == CB0R_NEG) {
        at += item.value;
        memmove(at+1,at,outlen-(at-out));
        at[0] = ' ';
        at++;
        outlen++;
        continue;
      }

      // insert variable lengths
      if(item.type == CB0R_INT) {
        at += item.value;
        cb0r(res.start + res.header, res.start + res.header + res.length, i++, &item);
        // repeating whitespaces
        if(item.type == CB0R_NEG) {
          memmove(at + item.value, at, outlen - (at - out));
          memset(at,' ',item.value);
          at += item.value;
          outlen += item.value;
          continue;
        }
        if(item.type == CB0R_INT && item.value < 24) {
          uint8_t len = ws_tablen[item.value]/2;
          memmove(at + len, at, outlen - (at - out));
          base16_decode(ws_table[item.value],ws_tablen[item.value],(uint8_t*)at);
          at += len;
          outlen += len;
          continue;
        }
      }

      break;
    }

  }

  return outlen;
}

/*
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

  // convert to temporary json buffer
  uint8_t *buf = malloc(inlen*2); 
  uint32_t len = snprintf((char *)buf, inlen * 2, "{\"protected\":\"%.*s\",\"payload\":\"%.*s\",\"signature\":\"%.*s\",}", (int)((dot1 - encoded) - 1), encoded, (int)((dot2 - dot1) - 1), dot1, (int)((end - dot2) + 1), dot2);
printf("encoding: %s\n",(char*)buf);
  // normal json->jscn conversion
  len = jscn_parse(buf,len,out,dict);
  free(buf);

  return len;
}
*/