#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

// struct to manage working state of JSCN data
typedef struct jscn_s {
  uint32_t quota; // total space available in buffer
  cb0r_s data; // always the first non-tag'd data item
  uint8_t buffer[]; // CBOR buffer space
} jscn_s, *jscn_t;

// NOTE: all methods that return pointers must be free'd by caller

///////// general methods
//
// contained data item (skips any JSCN tags) is filled into result
bool jscn_getdata(jscn_t jscn, cb0r_t result);

// returns exported raw ref-condensed CBOR
cb0r_t jscn_export(jscn_t jscn, bool (*ref_lookup)(cb0r_t id, cb0r_t refs));

// loads raw CBOR and validates as JSCN, expands all references
jscn_t jscn_load(cb0r_t cbor, bool (*ref_lookup)(cb0r_t id, cb0r_t refs));

///////// JSON -> JSCN methods
//
// recodes raw JSON into JSCN, includes additional buffer for optional refs/whitespace tags
jscn_t jscn_json2(char *json, uint32_t len);

// adds a tag 42 and optional refs id
bool jscn_addrefs(jscn_t jscn, cb0r_t id);

// captures any non-semantic whitespace into hints, only adds tag 1764 array if any found
bool jscn_addws(jscn_t jscn, char *json, uint32_t len);


////////// JSCN -> JSON methods
//
// converts JSCN back into null-terminated JSON, adds whitespace if hints found
char *jscn_2json(jscn_t jscn);


