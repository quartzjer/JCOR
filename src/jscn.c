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
  return NULL;
}

// loads raw CBOR and validates as JSCN, expands all references
jscn_t jscn_load(cb0r_t cbor, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  assert(cbor);
  if(cbor->type >= CB0R_ERR) return false;

  // iterate through

  return true;
}
