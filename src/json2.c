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

#include "base16.h"
#include "base64.h"
#include "js0n.h"
#include "jscn.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// match raw cbor value in array and return index (-1 if none)
static int32_t get_index(cb0r_t array, uint8_t *str, uint32_t len)
{
  if(!array || !str || !len) return -1;
  for(uint32_t i = 0; i < array->count; i++) {
    cb0r_s res;
    if(!cb0r(cb0r_value(array), array->end, i, &res)) break;
    if(cb0r_vlen(&res) != len) continue;
    if(memcmp(str, cb0r_value(&res), len) == 0) return i;
  }
  return -1;
}

static uint8_t *on2cn_part(uint8_t *out, uint8_t *in, size_t inlen, bool iskey, cb0r_t refs)
{
  if(in[0] == '{' || in[0] == '[') {
    // count items, write cbor map/array+count, recurse each k/v
    uint16_t i = 0;
    size_t len = 0;
    for(; js0n(NULL, i, (char *)in, inlen, &len, NULL); i++)
      ;
    if(in[0] == '{') {
      out += cb0r_write(out, CB0R_MAP, i / 2);
    } else {
      out += cb0r_write(out, CB0R_ARRAY, i);
    }
    for(uint16_t j = 0; j < i; j++) {
      const char *str = js0n(NULL, j, (char *)in, inlen, &len, NULL);
      out = on2cn_part(out, (uint8_t *)str, len, (in[0] == '{' && (j & 1) == 0), refs);
    }

  } else if(in[inlen] == '"') { // js0n type detection pattern :/

    // base64 value check
    uint32_t blen = 0;
    uint8_t *buff = out + 32; // temporary buffer padded out to leave room for writing tags
    if(!iskey && inlen > 10 && (blen = base64_decoder((char *)in, inlen, buff))) {
      // validate exact match using out as buffer
      base64_encoder(buff, blen, (char *)(buff + blen));
      if(memcmp(buff + blen, in, inlen) == 0) {
        out += cb0r_write(out, CB0R_TAG, 21);

        // check if the decoded is itself JSON 
        jscn_t jscn = jscn_json2((char*)buff, blen, refs, true);
        char *json = jscn_2json(jscn, refs, true);
        if(json && memcmp(buff, json, blen) == 0) {
          // TODO support auto between raw data and tagged
          uint32_t blen2 = jscn->data.end - jscn->data.start;
          printf("recursed %u < %u: %s\n", blen2, blen, json);
          memcpy(out, jscn->data.start, blen2);
          out += blen2;
          free(jscn);
          free(json);
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
    if(!iskey && base16_check((char *)in, inlen)) {
      blen = inlen / 2;
      out += cb0r_write(out, CB0R_TAG, 23);
      out += cb0r_write(out, CB0R_BYTE, blen);
      base16_decode((char *)in, inlen, out);
      out += blen;
      return out;
    }

    // check dictionary for UTF8 swap
    int32_t index;
    if(refs && (index = get_index(refs, in, inlen)) && index > 0 && index < 256) {
      // cbor byte key to represent value
      out += cb0r_write(out, CB0R_BYTE, 1);
      out[0] = index;
      out++;
    } else {
      // write CBOR UTF8
      out += cb0r_write(out, CB0R_UTF8, inlen);
      memcpy(out, in, inlen);
      out += inlen;
    }

  } else if(memcmp(in, "false", inlen) == 0) {
    out += cb0r_write(out, CB0R_SIMPLE, 20);
  } else if(memcmp(in, "true", inlen) == 0) {
    out += cb0r_write(out, CB0R_SIMPLE, 21);
  } else if(memcmp(in, "null", inlen) == 0) {
    out += cb0r_write(out, CB0R_SIMPLE, 22);
  } else if(memchr(in, '.', inlen) || memchr(in, 'e', inlen) || memchr(in, 'E', inlen)) {
    // TODO write cbor tag 4 for decimal floats/exponents
  } else {
    // parse number, write cbor int
    long long num = strtoll((const char *)in, NULL, 10);
    if(num < 0)
      out += cb0r_write(out, CB0R_NEG, (uint64_t)(~num));
    else
      out += cb0r_write(out, CB0R_INT, (uint64_t)num);
  }

  return out;
}

static uint8_t *ws2cn(uint8_t *out, uint8_t *in, size_t inlen, uint32_t *count)
{
  // get all whitespace pointers
  char **whitespace = malloc((inlen + 1) * sizeof(char *)); // temporary array of char*'s
  memset(whitespace, 0, (inlen + 1) * sizeof(char *)); // js0n fills only nulls
  whitespace[inlen] = "end";
  size_t err = 0;
  js0n("\0", 1, (char *)in, inlen, &err, whitespace);
  whitespace[inlen] = NULL; // ensure terminated

  // fill in array w/ integers
  uint32_t prev = 0;
  *count = 0;
  for(char **i = whitespace; *i;) {
    // start/end are _inclusive_
    char **start = i, **end = i;
    for(i++; *i && (*i - *end) == 1; i++, end++)
      ;
    do {
      uint8_t *ws = (uint8_t *)*start;
      uint32_t wslen = (*end - *start) + 1;
      uint32_t offset = (ws - in) - prev;
      prev = ws - in;
      //      printf("WS off %u %.*s\n", offset, wslen * 2, base16_encode(ws, wslen, (char *)out));

      // single space special case first
      if(wslen == 1 && ws[0] == ' ') {
        out += cb0r_write(out, CB0R_NEG, offset);
        prev++;
        count[0]++;
        break;
      }

      // then add offset
      out += cb0r_write(out, CB0R_INT, offset);
      count[0]++;

      // count any repeating/leading spaces
      uint32_t spaces = 0;
      for(uint32_t j = 0; j < wslen; j++)
        if(ws[j] == ' ')
          spaces++;
        else
          break;
      if(spaces) {
        out += cb0r_write(out, CB0R_NEG, spaces);
        *start += spaces;
        prev += spaces;
        count[0]++;
        continue;
      }

      // look for the longest prefix from the table
      int8_t best = -1;
      for(uint8_t j = 0; j <= 23; j++) {
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
      count[0]++;

    } while(*start < *end);
  }

  free(whitespace);
  return out;
}

// recodes raw JSON into JSCN (caller must free returned pointer)
jscn_t jscn_json2(char *json, uint32_t len, cb0r_t refs, bool whitespace)
{
  // validate any json first w/ full scan by looking for invalid key
  size_t err = 0;
  char *ws[2] = { NULL, "end" }; // notice if there's any whitespace also
  js0n("\0", 1, json, len, &err, ws);
  if(err) return 0;
  if(!ws[0]) whitespace = false;

  // first make the JSCN tag and array
  uint8_t *out = malloc(len);
  uint8_t *at = out;
  at += cb0r_write(at, CB0R_TAG, 20);
  uint8_t items = 1;
  if(refs || whitespace) items++;
  if(whitespace) items++;
  at += cb0r_write(at, CB0R_ARRAY, items);

  // generate into value
  at = on2cn_part(at, (uint8_t *)json, len, false, refs);

  if(refs) {
    cb0r_s res;
    if(!cb0r_get(refs, 0, &res)) return 0;
    at += cb0r_write(at, CB0R_INT, res.value);
    printf("using dictionary %llu\n", res.value);
  }else if(whitespace) {
    at += cb0r_write(at, CB0R_INT, 0);
  }

  // add any whitespace
  if(whitespace) {
    // get count first for fixed array
    uint32_t count = 0;
    ws2cn(at, (uint8_t *)json, len, &count);
    at += cb0r_write(at, CB0R_ARRAY, count);
    at = ws2cn(at, (uint8_t *)json, len, &count);
  }

  return jscn_load(out, at - out);
}

