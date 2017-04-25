#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"
#include "js0n.h"
#include "base64.h"
#include "base16.h"

// shared with stringify
#define WS_MAXLEN 16
const char *ws_table[24] = { "\n", "\n  ", "\n    ", "\n      ", "\n        ", "\n          ", "\n            ", "\n              ", "\t", "\n\t", "\n\t\t", "\n\t\t\t", "\n\t\t\t\t", "\n\t\t\t\t\t", "\n\t\t\t\t\t\t", "\n\t\t\t\t\t\t\t", "\n\t\t\t\t\t\t\t\t", "\r", "\r\n", "\r\n  ", "\r\n    ", "\r\n\t", "\r\n\t\t", "\r\n\t\t\t" };
const uint8_t ws_tablen[24] = { 1, 3, 5, 7, 9, 11, 13, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 4, 6, 3, 4, 5 };

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

static uint32_t on2cn_wrap(char *json, uint32_t len, uint8_t *out);

static uint8_t *on2cn_part(uint8_t *out, uint8_t *in, size_t inlen, bool iskey)
{
  if(in[0] == '{' || in[0] == '[')
  {
    // count items, write cbor map/array+count, recurse each k/v
    uint16_t i=0;
    size_t len = 0;
    for(;js0n(NULL,i,(char*)in,inlen,&len,NULL);i++);
    if(in[0] == '{')
    {
      out += cb0r_write(out, CB0R_MAP, i / 2);
    } else {
      out += cb0r_write(out, CB0R_ARRAY, i);
    }
    for(uint16_t j=0;j<i;j++)
    {
      const char *str = js0n(NULL,j,(char*)in,inlen,&len,NULL);
      out = on2cn_part(out, (uint8_t *)str, len, (in[0] == '{' && (j & 1) == 0));
    }

  }else if(in[inlen] == '"'){ // js0n type detection pattern :/

    // base64 value check
    uint32_t blen = 0;
    uint8_t *buff = out + 32; // temporary buffer padded out to leave room for writing tags
    if(!iskey && inlen > 10 && (blen = base64_decoder((char *)in, inlen, buff))) {
      // validate exact match using out as buffer
      base64_encoder(buff, blen, (char *)(buff + blen));
      if(memcmp(buff + blen, in, inlen) == 0) {
        out += cb0r_write(out, CB0R_TAG, 21);

        // check if the decoded is itself JSON
        uint32_t blen2 = on2cn_wrap((char*)buff, blen, buff + blen);
        if(blen2 && blen2 < blen){
          printf("recursed %u < %u: %.*s\n",blen2,blen,blen,(char*)buff);
          memmove(out, buff + blen, blen2);
          out += blen2;
          return out;
        }

        // if just base64 insert decoded byte string
        out += cb0r_write(out, CB0R_BYTE, blen);
        memmove(out, buff, blen);
        out += blen;
        return out;
      }
      // not b64, fall through
    }

    // base16 value check
    if(!iskey && base16_check((char*)in,inlen))
    {
      blen = inlen/2;
      out += cb0r_write(out, CB0R_TAG, 23);
      out += cb0r_write(out, CB0R_BYTE, blen);
      base16_decode((char *)in, inlen, out);
      out += blen;
      return out;
    }

    // write CBOR UTF8
    out += cb0r_write(out, CB0R_UTF8, inlen);
    memcpy(out, in, inlen);
    out += inlen;

  }else if(memcmp(in,"false",inlen) == 0){
    out += cb0r_write(out, CB0R_SIMPLE, 20);
  }else if(memcmp(in,"true",inlen) == 0){
    out += cb0r_write(out, CB0R_SIMPLE, 21);
  }else if(memcmp(in,"null",inlen) == 0){
    out += cb0r_write(out, CB0R_SIMPLE, 22);
  }else if(memchr(in,'.',inlen) || memchr(in,'e',inlen) || memchr(in,'E',inlen)){
    // TODO write cbor tag 4 for decimal floats/exponents
  }else{
    // parse number, write cbor int
    long long num = strtoll((const char*)in,NULL,10);
    if(num < 0)
      out += cb0r_write(out, CB0R_NEG, (uint64_t)(~num));
    else
      out += cb0r_write(out, CB0R_INT, (uint64_t)num);
  }
  return out;
}

static void ws2cn(uint8_t *out, uint8_t *in, size_t inlen)
{
  // get all whitespace pointers
  char **whitespace = malloc((inlen + 1) * sizeof(char *)); // temporary array of char*'s
  memset(whitespace, 0, (inlen + 1) * sizeof(char *)); // js0n fills only nulls
  whitespace[inlen] = "end";
  size_t err = 0;
  js0n("\0", 1, (char *)in, inlen, &err, whitespace);
  whitespace[inlen] = NULL; // ensure terminated

  // indefinite length array
  out[0] = CB0R_ARRAY << 5;
  out[0] |= 31;
  out++;

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
//      printf("WS off %u %.*s\n", offset, wslen * 2, base16_encode(ws, wslen, (char *)out));

      // single space special case first
      if(wslen == 1 && ws[0] == ' ')
      {
        out += cb0r_write(out, CB0R_NEG, offset);
        prev++;
        break;
      }

      // then add offset
      out += cb0r_write(out, CB0R_INT, offset);

      // count any repeating/leading spaces
      uint32_t spaces = 0;
      for(uint32_t j=0;j<wslen;j++) if(ws[j] == ' ') spaces++; else break;
      if(spaces) {
        out += cb0r_write(out, CB0R_NEG, spaces);
        *start += spaces;
        prev += spaces;
        continue;
      }

      // look for the longest prefix from the table
      int8_t best = -1;
      for(uint8_t j=0;j<=23;j++) {
        // the current ws must match 
        if(ws_tablen[j] > wslen) continue;
        if(memcmp(ws, ws_table[j], ws_tablen[j]) != 0) continue;
        best = j;
      }
      if(best == -1) printf("FATAL ERROR: [%.*s]\n", wslen, ws);

      // save table id and advance
      out += cb0r_write(out, CB0R_INT, best);
      *start += ws_tablen[best];
      prev += ws_tablen[best];

    } while(*start < *end);
  }

  // add break to end of indefinite length array
  out[0] = 0xff;
  out++;

  free(whitespace);
}

static uint32_t on2cn_wrap(char *json, uint32_t len, uint8_t *out)
{
  // validate any json first w/ full scan by looking for invalid key
  if(!json || !len || (json[0] != '{' && json[0] != '[')) return 0;
  size_t err = 0;
  js0n("\0", 1, json, len, &err, NULL);
  if(err) return 0;

  uint8_t *end = on2cn_part(out, (uint8_t *)json, len, false);

  return end - out;
}

// recodes raw JSON into JSCN, includes additional buffer for optional refs/whitespace tags
jscn_t jscn_json2(char *json, uint32_t len)
{
  if(!json || !len) return NULL;

  // more than enough space for now
  jscn_t jscn = malloc(sizeof(jscn_s) + (len * 1.4));
  memset(jscn, 0, sizeof(jscn_s) + (len * 1.4));
  jscn->quota = (len * 1.4) - sizeof(jscn_s);

  jscn->len = on2cn_wrap(json, len, jscn->buffer);
  if(!jscn->len) {
    free(jscn);
    return NULL;
  }

  return jscn;
}

// adds a tag 42 and optional refs id
bool jscn_addrefs(jscn_t jscn, cb0r_t id)
{
  // TODO move forward 2, array len 1 or 2, add id
  return false;
}

// captures any non-semantic whitespace into hints, only adds tag 1764 array if any found
bool jscn_addws(jscn_t jscn, char *json, uint32_t len)
{
  // notice if there's any whitespace 
  char *ws[2] = { NULL, "end" };
  size_t err = 0;
  js0n("\0", 1, json, len, &err, ws);
  if(err) return false;
  if(!ws[0]) return false;

  // TODO move forward for tag and array, write array
  //state.out += cb0r_write(state.out, CB0R_INT, JSCN_KEY_WS);
  //ws2cn(&state, (uint8_t *)json, len);

  return true;
}
