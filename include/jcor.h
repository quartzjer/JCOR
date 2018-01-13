#ifndef jcor_h
#define jcor_h

#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jcor_s {
  uint8_t *start;
  uint32_t length;
  cb0r_s tag; // JCOR tag + array
  cb0r_s data; // the constrained JSON data
  int32_t refs; // refs id being used
  cb0r_s ws; // whitespace array (if any)
} jcor_s, *jcor_t;

// recodes raw JSON into JCOR (caller must free ret and ret->start pointers)
jcor_t jcor_json2(char *json, uint32_t len, cb0r_t refs, bool whitespace);

// converts JCOR back into new null-terminated JSON, adds back whitespace if requested
char *jcor_2json(jcor_t jcor, cb0r_t refs, bool whitespace);

// loads raw CBOR and validates as JCOR, caller must free result
jcor_t jcor_load(uint8_t *cbor, uint32_t len);

#endif // jcor_h