#include <stdint.h>
#include <stdbool.h>

size_t js2cb(uint8_t *in, size_t inlen, uint8_t *out, bool iskey);
size_t cb2js(uint8_t *in, size_t inlen, char *out, uint32_t skip);
