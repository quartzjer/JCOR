#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jcor.h"
#include "base64.h"
#include "base16.h"

// shared with parse
#define WS_MAXLEN 16
extern const char *ws_table[24];
extern const uint8_t ws_tablen[24];

static size_t cn2on_part(uint8_t *in, size_t inlen, char *out, uint32_t skip, cb0r_t refs, bool whitespace)
{
  size_t outlen = 0;
  cb0r_s res;
  uint8_t *end = cb0r(in, in + inlen, skip, &res);

  // dictionary replacement swap
  if(res.type == CB0R_BYTE && res.length == 1 && (!refs || !cb0r_get(refs, res.start[res.header], &res))) {
    printf("not found in dictionary: %u\n", res.start[res.header]);
    return 0;
  }

  // expand by type
  switch(res.type) {
    case CB0R_INT: {
      outlen = sprintf(out, "%llu", res.value);
    } break;
    case CB0R_NEG: {
      outlen = sprintf(out, "-%llu", res.value + 1);
    } break;
    case CB0R_UTF8: {
      outlen = sprintf(out, "\"%.*s\"", (int)res.length, cb0r_value(&res));
    } break;
    case CB0R_ARRAY:
    case CB0R_MAP: {
      outlen += sprintf(out + outlen, (res.type == CB0R_MAP) ? "{" : "[");
      for(uint32_t i = 0; i < res.count; i++) {
        if(i) outlen += sprintf(out + outlen, ",");
        outlen += cn2on_part(cb0r_value(&res), cb0r_vlen(&res), out + outlen, i, refs, whitespace);
        if(res.type != CB0R_MAP) continue;
        outlen += sprintf(out + outlen, ":");
        outlen += cn2on_part(cb0r_value(&res), cb0r_vlen(&res), out + outlen, ++i, refs, whitespace);
      }
      outlen += sprintf(out + outlen, (res.type == CB0R_MAP) ? "}" : "]");
    } break;
    case CB0R_BASE64URL: {
      out[0] = '"';
      cb0r_s res2;
      cb0r(cb0r_value(&res), end, 0, &res2);
      if(res2.type == CB0R_BYTE) {
        outlen = base64_encoder(cb0r_value(&res2), res2.length, out + 1);
      } else {
        printf("recursing len %u\n", cb0r_vlen(&res));
        outlen = cn2on_part(cb0r_value(&res), cb0r_vlen(&res), out + cb0r_vlen(&res), 0, refs, whitespace);
        outlen = base64_encoder((uint8_t *)(out + cb0r_vlen(&res)), outlen, out + 1);
      }
      out[outlen + 1] = '"';
      outlen += 2;
    } break;
    case CB0R_HEX: {
      cb0r_s res2;
      cb0r(cb0r_value(&res), end, 0, &res2);
      if(res2.type == CB0R_BYTE) {
        out[0] = '"';
        outlen = res2.length * 2;
        base16_encode(cb0r_value(&res2), res2.length, out + 1);
        out[outlen + 1] = '"';
        outlen += 2;
      }
    } break;
    case CB0R_FALSE: {
      outlen = sprintf(out, "false");
    } break;
    case CB0R_TRUE: {
      outlen = sprintf(out, "true");
    } break;
    case CB0R_NULL: {
      outlen = sprintf(out, "null");
    } break;
    default: {
      printf("unsupported type: %u\n", res.type);
    } break;
  }

  return outlen;
}

// converts JSCN back into null-terminated JSON, adds back whitespace if requested
char *jscn_2json(jscn_t jscn, cb0r_t refs, bool whitespace)
{
  if(!jscn) return NULL;

  char *json = malloc(jscn->length * 2);

  uint32_t outlen = cn2on_part(jscn->data.start, jscn->data.end - jscn->data.start, json, 0, refs, whitespace);
  json[outlen] = 0;

  // check for whitespace hints
  if(whitespace && jscn->ws.type == CB0R_ARRAY) {
    printf("using whitespace hints\n");
    cb0r_s item = {0,};
    uint32_t i = 0;
    char *at = json;
    while(item.type < CB0R_ERR) {
      cb0r(cb0r_value(&(jscn->ws)), jscn->ws.end, i++, &item);

      // insert single space first
      if(item.type == CB0R_NEG) {
        at += item.value;
        memmove(at + 1, at, outlen - (at - json));
        at[0] = ' ';
        at++;
        outlen++;
        continue;
      }

      // insert variable lengths
      if(item.type == CB0R_INT) {
        at += item.value;
        cb0r(cb0r_value(&(jscn->ws)), jscn->ws.end, i++, &item);
        // repeating whitespaces
        if(item.type == CB0R_NEG) {
          memmove(at + item.value, at, outlen - (at - json));
          memset(at, ' ', item.value);
          at += item.value;
          outlen += item.value;
          continue;
        }
        if(item.type == CB0R_INT && item.value < 24) {
          uint8_t len = ws_tablen[item.value];
          memmove(at + len, at, outlen - (at - json));
          memcpy((uint8_t *)at, ws_table[item.value], len);
          at += len;
          outlen += len;
          continue;
        }
      }

      break;
    }
  }

  return json;
}

