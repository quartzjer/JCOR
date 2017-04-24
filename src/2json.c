#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"
#include "base64.h"
#include "base16.h"

// shared with parse
#define WS_MAXLEN 16
extern const char *ws_table[24];
extern const uint8_t ws_tablen[24];

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
        outlen = jscn_stringify(&jscn, out+1);
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

// converts JSCN back into JSON (returns size required if json is NULL)
uint32_t jscn_stringify(jscn_t jscn, char *json)
{
  if(!jscn) return 0;

  if(!json) {
    printf("TODO\n");
    return 0;
  }

  // convert the raw data
  uint32_t outlen = cn2on_part(jscn->data.start, jscn->data.end - jscn->data.start, json, 0, jscn->dict);

  // check for whitespace hints
  cb0r_s res = {0,};
  if(cb0r_find(&(jscn->map), CB0R_INT, JSCN_KEY_WS, NULL, &res) && res.type == CB0R_ARRAY) {
    printf("using whitespace hints\n");
    cb0r_s item = {0,};
    uint32_t i = 0;
    char *at = json;
    while(item.type < CB0R_ERR) {
      cb0r(res.start + res.header, res.start + res.header + res.length, i++, &item);

      // insert single space first
      if(item.type == CB0R_NEG) {
        at += item.value;
        memmove(at+1,at,outlen-(at-json));
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
          memmove(at + item.value, at, outlen - (at - json));
          memset(at,' ',item.value);
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

  return outlen;
}

// converts JSCN back into null-terminated JSON, adds whitespace if hints found
char *jscn_2json(jscn_t jscn)
{
  return NULL;
}
