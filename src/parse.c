#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"
#include "js0n.h"
#include "base64.h"
#include "base16.h"

// internal working state
typedef struct state_s {
  uint8_t *start;
  uint8_t *out;
  jscn_t dict;
} state_s, *state_t;

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

static uint32_t on2cn_wrap(char *json, uint32_t len, uint8_t *out, jscn_t dict);

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
        uint32_t blen2 = on2cn_wrap((char*)buff, blen, buff + blen, state->dict);
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
      int8_t best = -1;
      for(uint8_t j=0;j<=23;j++) {
        // the current ws must match 
        if(ws_tablen[j] > wslen) continue;
        if(memcmp(ws, ws_table[j], ws_tablen[j]) != 0) continue;
        best = j;
      }
      if(best == -1) printf("FATAL ERROR: [%.*s]\n", wslen, ws);

      // save table id and advance
      state->out += cb0r_write(state->out, CB0R_INT, best);
      *start += ws_tablen[best];
      prev += ws_tablen[best];

    } while(*start < *end);
  }

  // add break to end of indefinite length array
  state->out[0] = 0xff;
  state->out++;

  free(whitespace);
}

static uint32_t on2cn_wrap(char *json, uint32_t len, uint8_t *out, jscn_t dict)
{
  // validate any json first w/ full scan by looking for invalid key
  if(!json || !len || (json[0] != '{' && json[0] != '[')) return 0;
  size_t err = 0;
  char *ws[2] = {NULL,"end"}; // notice if there's any whitespace also
  js0n("\0", 1, json, len, &err, ws);
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
  on2cn_part(&state, (uint8_t *)json, len, false);

  // add any whitespace
  if(ws[0]) {
    state.out += cb0r_write(state.out, CB0R_INT, JSCN_KEY_WS);
    ws2cn(&state, (uint8_t *)json, len);
  }

  return state.out - out;
}

// parses raw JSON into JSCN (jscn buffer must be 2*len)
bool jscn_parse(char *json, uint32_t len, uint8_t *jscn, jscn_t result)
{
  assert(jscn);
  assert(result);
  if(!json || !len) return false;

  uint32_t jscn_len = on2cn_wrap(json, len, jscn, result->dict);
  if(!jscn_len) return false;

  return jscn_load(jscn, jscn_len, result);
}
