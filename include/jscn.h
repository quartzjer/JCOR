#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jscn_s {
  struct jscn_s *dict; // optional pointer to dictionary
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
static char *ws_table[24] = { "0a", "0a2020", "0a20202020", "0a202020202020", "0a2020202020202020", "0a20202020202020202020", "0a202020202020202020202020", "0a2020202020202020202020202020", "09", "0a09", "0a0909", "0a090909", "0a09090909", "0a0909090909", "0a090909090909", "0a09090909090909", "0a0909090909090909", "0d", "0d0a", "0d0a2020", "0d0a20202020", "0d0a09", "0d0a0909", "0d0a090909" };
static uint8_t ws_tablen[24] = { 2, 6, 10, 14, 18, 22, 26, 30, 2, 4, 6, 8, 10, 12, 14, 16, 18, 2, 4, 8, 12, 6, 8, 10 };
#define WS_MAXLEN 32

// named keys for JSCN map
enum {
  JSCN_KEY_DATA = 1,
  JSCN_KEY_DICT,
  JSCN_KEY_WS
};
