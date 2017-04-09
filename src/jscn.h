#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jscn_s {
  struct jscn_s *dict;
  cb0r_s jscn;
  cb0r_s data;
} jscn_s, *jscn_t;

// parses raw JSON into JSCN (jscn buffer must be 2*len)
bool jscn_parse(char *json, uint32_t len, uint8_t *jscn, jscn_t result);

// converts JSCN back into JSON (returns size required if json is NULL)
uint32_t jscn_stringify(jscn_t jscn, char *json);

// validates JSCN and loads data, fills result if successful
bool jscn_load(uint8_t *jscn, uint32_t len, jscn_t result);

// fills dictionary id (if any), id will be UTF-8 string or integer
bool jscn_dictionary(jscn_t jscn, cb0r_t id);
