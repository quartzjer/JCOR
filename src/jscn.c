#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"

// loads raw CBOR and validates as JSCN, caller must free
jscn_t jscn_load(uint8_t *cbor, uint32_t len)
{
  if(!cbor || !len) return NULL;

  cb0r_s res;
  cb0r(cbor, cbor+len, 0, &res);
  if(res.type != CB0R_TAG || res.value != 20) {
    printf("CBOR is not tagged as JSCN: %u/%llu\n", res.type, res.value);
    return NULL;
  }

  cb0r(cb0r_value(&res), res.end, 0, &res);
  if(res.type != CB0R_ARRAY || !res.count || res.count > 3) {
    printf("invalid type following JSCN tag: %u/%llu\n", res.type, res.value);
    return false;
  }

  jscn_t jscn = malloc(sizeof(jscn_s));
  memset(jscn, 0, sizeof(jscn_s));
  jscn->start = cbor;
  jscn->length = len;
  cb0r(cbor, cbor + len, 0, &(jscn->tag));

  // extract parts of the array
  cb0r_get(&res, 0, &(jscn->data));

  if(res.count > 1) {
    cb0r_s tmp;
    cb0r_get(&res, 1, &tmp);
    jscn->refs = tmp.value;
  }

  if(res.count > 2) cb0r_get(&res, 2, &(jscn->ws));

  return jscn;
}
