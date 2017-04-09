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

/*
size_t jwt2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict)
{
  char *encoded = (char*)in;
  char *end = encoded+(inlen-1);
  
  // make sure the dot separators are there
  char *dot1 = memchr(encoded,'.',end-encoded);
  if(!dot1 || dot1+1 >= end) return 0;
  dot1++;
  char *dot2 = memchr(dot1,'.',end-dot1);
  if(!dot2 || (dot2+1) >= end) return 0;
  dot2++;

  // convert to temporary json buffer
  uint8_t *buf = malloc(inlen*2); 
  uint32_t len = snprintf((char *)buf, inlen * 2, "{\"protected\":\"%.*s\",\"payload\":\"%.*s\",\"signature\":\"%.*s\",}", (int)((dot1 - encoded) - 1), encoded, (int)((dot2 - dot1) - 1), dot1, (int)((end - dot2) + 1), dot2);
printf("encoding: %s\n",(char*)buf);
  // normal json->jscn conversion
  len = jscn_parse(buf,len,out,dict);
  free(buf);

  return len;
}
*/