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

// fill in value for matching key in given map
bool jscn_getkv(cb0r_t map, cb0r_t key, cb0r_t value);
