#include <stdint.h>
#include <stdbool.h>

size_t js2cb(uint8_t *in, size_t inlen, uint8_t *out, bool iskey);
size_t cb2js(uint8_t *in, size_t inlen, char *out, uint32_t skip);

size_t jwt2cb(uint8_t *in, size_t inlen, uint8_t *out);

// fetch utf8 string value at given index of cbor array
uint8_t *cb_getv(uint8_t *in, size_t inlen, uint32_t index, size_t *len);

// match string value in array and return index (-1 if none)
int32_t cb_geti(uint8_t *in, size_t inlen, uint8_t *value, size_t len);

// app defines these to resolve a dictionary by id or by json object
__weak uint8_t *dict_id(int32_t id, size_t *len);
__weak uint8_t *dict_match(uint8_t *obj, size_t objlen, size_t *len);
