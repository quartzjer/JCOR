#include <string.h>
#include <stdint.h>
#include <stdlib.h>

char *base16_encode(uint8_t *in, size_t len, char *out)
{
    uint32_t j;
    char *c = out;
    static char *hex = "0123456789abcdef";
    if(!in || !len || !out) return NULL;

    for (j = 0; j < len; j++) {
      *c = hex[((in[j]&240)/16)];
      c++;
      *c = hex[in[j]&15];
      c++;
    }

    return out;
}

static inline char hexcode(char x)
{
  if (x >= '0' && x <= '9')         /* 0-9 is offset by hex 30 */
    return (x - 0x30);
  else if (x >= 'a' && x <= 'f')    /* a-f offset by hex 37 */
    return (x - 0x57);
  else                              /* Otherwise, an illegal hex digit */
    return x;
}

uint8_t *base16_decode(char *in, size_t len, uint8_t *out)
{
  uint32_t j;
  uint8_t *c = out;
  if(!out || !in) return NULL;
  if(!len) len = strlen(in);

  for(j=0; (j+1)<len; j+=2)
  {
    *c = ((hexcode(in[j]) * 16) & 0xF0) + (hexcode(in[j+1]) & 0xF);
    c++;
  }
  return out;
}

char *base16_check(char *str, uint32_t len)
{
  if(!str || !len) return NULL;
  if(len & 1) return NULL; // must be even
  uint32_t i;
  for(i=0;i<len;i++)
  {
    if(!str[i]) return NULL;
    if(str[i] == hexcode(str[i])) return NULL;
  }
  return str;
}

