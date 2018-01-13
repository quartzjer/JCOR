#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jcor.h"

// loads raw CBOR and validates as JCOR, caller must free
jcor_t jcor_load(uint8_t *cbor, uint32_t len)
{
  if(!cbor || !len) return NULL;

  cb0r_s res;
  cb0r(cbor, cbor+len, 0, &res);
  if(res.type != CB0R_TAG || res.value != 20) {
    printf("CBOR is not tagged as JCOR: %u/%llu\n", res.type, res.value);
    return NULL;
  }

  cb0r(cb0r_value(&res), res.end, 0, &res);
  if(res.type != CB0R_ARRAY || !res.count || res.count > 3) {
    printf("invalid type following JCOR tag: %u/%llu\n", res.type, res.value);
    return false;
  }

  jcor_t jcor = malloc(sizeof(jcor_s));
  memset(jcor, 0, sizeof(jcor_s));
  jcor->start = cbor;
  jcor->length = len;
  cb0r(cbor, cbor + len, 0, &(jcor->tag));

  // extract parts of the array
  cb0r_get(&res, 0, &(jcor->data));

  if(res.count > 1) {
    cb0r_s tmp;
    cb0r_get(&res, 1, &tmp);
    jcor->refs = tmp.value;
  }

  if(res.count > 2) cb0r_get(&res, 2, &(jcor->ws));

  return jcor;
}
