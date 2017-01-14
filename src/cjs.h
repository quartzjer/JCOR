#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

size_t js2cb(uint8_t *in, size_t inlen, uint8_t *out, bool iskey, cb0r_t dict);
size_t cb2js(uint8_t *in, size_t inlen, char *out, uint32_t skip);

size_t jwt2cb(uint8_t *in, size_t inlen, uint8_t *out);

// fetch utf8 string value at given index of cbor array
bool cb_getv(cb0r_t array, uint32_t index, cb0r_t val);

// match string value in array and return index (-1 if none)
int32_t cb_geti(cb0r_t array, uint8_t *str, uint32_t len);

// app defines these to resolve a dictionary by id or by json object
__weak cb0r_t dict_id(int32_t id);
__weak cb0r_t dict_match(uint8_t *obj, size_t objlen);
