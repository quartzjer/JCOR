#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"

// returns the contained data item (skips any JSCN tags)
cb0r_t jscn_getdata(jscn_t jscn)
{
  if(!jscn) return NULL;
  return &(jscn->data);
}

// returns exported raw ref-condensed CBOR
cb0r_t jscn_export(jscn_t jscn, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  // create larger temporary buffer and write into it
  // create cb0r_s + exact size and return it
  // TODO when encountering any embedded structure, ask ref_lookup for array of id's and try all for best reduction
  /*
      if(state->dict && (index = get_index(&(state->dict->data), in, inlen)) && index > 0 && index < 256) {
      // cbor byte key to represent value
      out += cb0r_write(out, CB0R_BYTE, 1);
      out[0] = index;
      out++;

  */
  return NULL;
}

// loads raw CBOR and validates as JSCN, expands all references
jscn_t jscn_load(cb0r_t cbor, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  assert(cbor);
  if(cbor->type >= CB0R_ERR) return false;

  // iterate through

  /*
  // dictionary replacement swap
  if(res.type == CB0R_BYTE && res.length == 1 && (!dict || !cb0r_get(&(dict->data), res.start[res.header], &res)))
  {
    printf("not found in dictionary: %u\n",res.start[res.header]);
    return 0;
  }
  */

  return NULL;
}
