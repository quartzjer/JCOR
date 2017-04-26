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

  // TODO make jscn, fill in
  /*
  cb0r(res.start + res.header, res.end, 0, &(result->map));
  if(result->map.type != CB0R_MAP || !result->map.count) {
    printf("JSCN does not begin with a map: %u\n", result->map.type);
    return false;
  }

  if(!cb0r_find(&(result->map), CB0R_INT, JSCN_KEY_DATA, NULL, &(result->data))) {
    printf("JSCN does not contain data");
    return false;
  }
*/
  return NULL;
}
