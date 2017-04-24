#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jscn.h"

// contained data item (skips any JSCN tags) is filled into result
bool jscn_getdata(jscn_t jscn, cb0r_t result)
{
  return false;
}

// returns exported raw ref-condensed CBOR
cb0r_t jscn_export(jscn_t jscn, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  return NULL;
}

// loads raw CBOR and validates as JSCN, expands all references
jscn_t jscn_load(cb0r_t cbor, bool (*ref_lookup)(cb0r_t id, cb0r_t refs))
{
  assert(cbor);
  if(cbor->type >= CB0R_ERR) return false;

  // TODO

  return true;
}
