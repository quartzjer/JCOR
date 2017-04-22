#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"

// validates JSCN and loads data, fills result if successful
bool jscn_load(uint8_t *jscn, uint32_t len, jscn_t result)
{
  assert(result);
  if(!jscn || !len) return false;

  cb0r_s res = {0,};
  cb0r(jscn, jscn + len, 0, &res);
  if(res.type != CB0R_TAG || res.value != 42) {
    printf("CBOR is not tagged as JSCN: %u/%llu\n", res.type, res.value);
    return false;
  }

  cb0r(res.start + res.header, res.end, 0, &(result->map));
  if(result->map.type != CB0R_MAP || !result->map.count) {
    printf("JSCN does not begin with a map: %u\n", result->map.type);
    return false;
  }

  if(!cb0r_find(&(result->map), CB0R_INT, JSCN_KEY_DATA, NULL, &(result->data))) {
    printf("JSCN does not contain data");
    return false;
  }

  return true;
}
