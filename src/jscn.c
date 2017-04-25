#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"

// returns the contained data item (skips any JSCN tags)
cb0r_t jscn_getdata(jscn_t jscn)
{
  if(!jscn) return NULL;
  return &(jscn->data);
}

static uint8_t *export_part(uint8_t *in, size_t inlen, uint8_t *out, uint32_t skip)
{
  size_t outlen = 0;
  cb0r_s res = {0,};
  uint8_t *end = cb0r(in, in + inlen, skip, &res);

  // expand by type
  switch(res.type) {
    case CB0R_UTF8: {
      // reference check 
      break;
    }
    case CB0R_ARRAY:
    case CB0R_MAP: {
      for(uint32_t i = 0; i < res.count; i++) {
        if(i) outlen += sprintf(out + outlen, ",");
        outlen += cn2on_part(res.start + res.header, end - (res.start + res.header), out + outlen, i);
        if(res.type != CB0R_MAP) continue;
        outlen += sprintf(out + outlen, ":");
        outlen += cn2on_part(res.start + res.header, end - (res.start + res.header), out + outlen, ++i);
      }
      outlen += sprintf(out + outlen, (res.type == CB0R_MAP) ? "}" : "]");
    } break;
    case CB0R_INT:
    case CB0R_NEG:
    case CB0R_BASE64URL: {
      out[0] = '"';
      cb0r_s res2 = {
        0,
      };
      cb0r(res.start + res.header, end, 0, &res2);
      if(res2.type == CB0R_BYTE) {
        outlen = base64_encoder(res2.start + res.header, res2.length, out + 1);
      } else {
        printf("recursing len %lu\n", res.end - (res.start + res.header));
        outlen = cn2on_part(res.start + res.header, end - (res.start + res.header), out + res2.length, 0);
        outlen = base64_encoder((uint8_t *)(out + res2.length), outlen, out + 1);
      }
      out[outlen + 1] = '"';
      outlen += 2;
    } break;
    case CB0R_HEX: {
      cb0r_s res2 = {
        0,
      };
      cb0r(res.start + res.header, end, 0, &res2);
      if(res2.type == CB0R_BYTE) {
        out[0] = '"';
        outlen = res2.length * 2;
        base16_encode(res2.start + res.header, res2.length, out + 1);
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

// returns exported raw ref-condensed CBOR
cb0r_t jscn_export(jscn_t jscn, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  cb0r_t cbor = malloc(sizeof(cbor_s) + jscn->len);
  memset(cbor, 0, sizeof(cbor_s) + jscn->len);

  uint8_t *out = (uint8_t*)(cbor) + sizeof(cbor_s);
  uint8_t *in = jscn->buffer;
  for()
      cb0r_s res = {0,};
  uint8_t *end = cb0r(in, in + inlen, skip, &res);

  // TODO when encountering any embedded structure, ask ref_lookup for array of id's and try all for best reduction
  /*
      if(state->dict && (index = get_index(&(state->dict->data), in, inlen)) && index > 0 && index < 256) {
      // cbor byte key to represent value
      out += cb0r_write(out, CB0R_BYTE, 1);
      out[0] = index;
      out++;

  */
  return cbor;
}

// loads raw CBOR and validates as JSCN, expands all references
jscn_t jscn_load(cb0r_t cbor, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  assert(cbor);
  if(cbor->type >= CB0R_ERR) return false;

  // iterate through

  /*
  // dictionary replacement swap
  if(res.type == CB0R_BYTE && res.length == 1 && (!dict || !cb0r_get(&(dict->data), res.start[res.header], &res)))
  {
    printf("not found in dictionary: %u\n",res.start[res.header]);
    return 0;
  }
  */

  return NULL;
}
