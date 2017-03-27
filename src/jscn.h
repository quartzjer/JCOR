#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

size_t json2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict);
size_t jscn2on(uint8_t *in, size_t inlen, char *out, cb0r_t dict);

size_t jwt2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict);

// fetch utf8 string value at given index of cbor array
bool jscn_getv(cb0r_t array, uint32_t index, cb0r_t val);

// match cbor item in array and return index (-1 if none)
int32_t jscn_geti(cb0r_t array, cb0r_t item);

// fill in value for matching key in given map, returns value's index
uint32_t jscn_getkv(cb0r_t map, cb0r_t key, cb0r_t value);

// validates jscn and returns the given index value from the wrapper
typedef enum {
  JSCN_KEY_DATA = 1,
  JSCN_KEY_DICT,
  JSCN_KEY_WS
} jscnkey_e;
uint32_t jscn2cbor(uint8_t *in, size_t inlen, cb0r_t res, jscnkey_e key, cb0r_t value);
