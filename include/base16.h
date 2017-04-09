#ifndef base16_h
#define base16_h

#include <stdint.h>

// make sure out is 2*len 
char *base16_encode(uint8_t *in, size_t len, char *out);

// out must be len/2
uint8_t *base16_decode(char *in, size_t len, uint8_t *out);

// hex string validator, NULL is invalid, else returns str
char *base16_check(char *str, uint32_t len);

#endif
