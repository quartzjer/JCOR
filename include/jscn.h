#ifndef jscn_h
#define jscn_h

#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jscn_s {
  uint8_t *start;
  uint32_t length;
  cb0r_s tag; // JSCN tag + array
  cb0r_s data; // the constrained JSON data
  int32_t refs; // refs id being used
  cb0r_s ws; // whitespace array (if any)
} jscn_s, *jscn_t;

// recodes raw JSON into JSCN (caller must free returned pointer)
jscn_t jscn_json2(char *json, uint32_t len, cb0r_t refs, bool whitespace);

// converts JSCN back into null-terminated JSON, adds back whitespace if requested
char *jscn_2json(jscn_t jscn, cb0r_t refs, bool whitespace);

// loads raw CBOR and validates as JSCN, caller must free result
jscn_t jscn_load(uint8_t *cbor, uint32_t len);

#endif // jscn_h