#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jscn_s {
  struct jscn_s *dict; // optional pointer to dictionary
  cb0r_s jscn; // full CBOR envelop
  cb0r_s map; // the JSCN wrapper map
  cb0r_s data; // the contained CBOR data
} jscn_s, *jscn_t;

// parses raw JSON into CBOR
bool jscn_parse(char *json, uint32_t len, jscn_t result);

// replaces any string keys in dictionary, result is tag 42
bool jscn_replace(jscn_t data, jscn_t dictionary, jscn_t result);

// captures any non-semantic whitespace into hints, result is tag 1764 array
bool jscn_whitespace(char *json, uint32_t len, jscn_t data, jscn_t result);

// converts JSCN back into JSON (returns size required if json is NULL)
uint32_t jscn_stringify(jscn_t jscn, char *json);

// resolve any dictionary references back into strings
bool jscn_lookup(jscn_t data, jscn_t dictionary, jscn_t result);

// expands json string with non-structural whitespace based on hints array
bool jscn_hints(char *json, jscn_t hints);

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

