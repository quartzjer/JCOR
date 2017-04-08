#include <stdint.h>
#include <stdbool.h>
#include "cb0r.h"

typedef struct jscn_s {
  struct jscn_s *dict;
  cb0r_s jscn;
  cb0r_s data;
} jscn_s, *jscn_t;

size_t jscn_parse(uint8_t *in, size_t inlen, uint8_t *out, jscn_t dict);

size_t jscn_stringify(jscn_t jscn, char *out, size_t len);

bool jscn_load(uint8_t *in, size_t inlen, jscn_t result);
