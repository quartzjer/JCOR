#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

size_t json2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict);
size_t jscn2on(uint8_t *in, size_t inlen, char *out, uint32_t skip, cb0r_t dict);

size_t jwt2cn(uint8_t *in, size_t inlen, uint8_t *out, cb0r_t dict);

// fetch utf8 string value at given index of cbor array
bool jscn_getv(cb0r_t array, uint32_t index, cb0r_t val);

// match cbor item in array and return index (-1 if none)
int32_t jscn_geti(cb0r_t array, cb0r_t item);

// app defines these to resolve a dictionary by id or by json object
__weak cb0r_t jscn_dict_id(int32_t id);
__weak cb0r_t jscn_dict_match(uint8_t *obj, size_t objlen);
