#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jscn_s {
  struct jscn_s *dict; // optional pointer to dictionary
  cb0r_s jscn; // full CBOR envelop
  cb0r_s map; // the JSCN wrapper map
  cb0r_s data; // the contained CBOR data
} jscn_s, *jscn_t;

// parses raw JSON into JSCN (jscn buffer must be 2*len)
bool jscn_parse(char *json, uint32_t len, uint8_t *jscn, jscn_t result);

// converts JSCN back into JSON (returns size required if json is NULL)
uint32_t jscn_stringify(jscn_t jscn, char *json);

// validates JSCN and loads data, fills result if successful
bool jscn_load(uint8_t *jscn, uint32_t len, jscn_t result);

// fills dictionary id (if any), id will be UTF-8 string or integer
bool jscn_dictionary(jscn_t jscn, cb0r_t id);

// whitespace table (used by parse and stringify)
static const char *ws_table[24] = { "\n", "\n  ", "\n    ", "\n      ", "\n        ", "\n          ", "\n            ", "\n              ", "\t", "\n\t", "\n\t\t", "\n\t\t\t", "\n\t\t\t\t", "\n\t\t\t\t\t", "\n\t\t\t\t\t\t", "\n\t\t\t\t\t\t\t", "\n\t\t\t\t\t\t\t\t", "\r", "\r\n", "\r\n  ", "\r\n    ", "\r\n\t", "\r\n\t\t", "\r\n\t\t\t" };
static uint8_t ws_tablen[24] = { 1, 3, 5, 7, 9, 11, 13, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 4, 6, 3, 4, 5 };
#define WS_MAXLEN 16

// named keys for JSCN map
enum {
  JSCN_KEY_DATA = 1,
  JSCN_KEY_DICT,
  JSCN_KEY_WS
};

