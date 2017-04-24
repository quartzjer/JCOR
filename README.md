# JSCN - JSON Constrained Notation

# TODO

* move to a subset definition
* add a tag for a dictionary and another for whitespace
* both followed by a variable array
* data is always first item
* references

## Scope

A subset of CBOR that perfectly mirrors any JSON string into a highly condensed notation for constrained nodes and networks.

* Round-trip from JSON->CBOR->JSON must always result in indentical UTF-8 strings
* Optimizes for compact representations wherever possible without breaking symmetry
* The CBOR notation must be directly usable by constrained applications and easily adopted for all data encoding needs
* Only the JSON encoding is canonical, the CBOR encoder implementations must produce identical semantic structures but the actual byte string may have slight variations
* JSCN transcoder software must operate directly on the UTF-8 JSON string when round-trip symmetry is required, if operating with a parsed JSON value the round-trip can only guarantee semantic compatibility within its parsed context

## Proposal

* Direct serialization of JSON structures to the CBOR major object/array types 4 and 5, ordering of key/value pairs in maps/objects must always be preserved
* JSON `true`, `false`, and `null` are serialized to their CBOR type 7 simple values
* Any JSON exponent value is encoded as a CBOR exponent (tag 4), if the contained `e` symbol is upper-case in JSON the case tag (20) must also be used
* JSON numbers should be encoded as CBOR Integers (type 0 and 1) or Floats (type 7) and then tested for compatibility by round-tripping them back to a JSON number, any remaining incompatible numbers are encoded as Bigfloats (tag 5)
* All JSON strings must be round-trip tested for possible encodings (base64url, base64, and hexadecimal) by attempting to decode and re-encode them, if identical UTF-8 strings result the decoded value is tagged in CBOR with the encoding format (tags 21, 22, and 20/23), any remaining strings are preserved as a UTF-8 string (type 3)
* Any UTF-8 or decoded byte strings that begin with a JSON structure byte of '{' or '[' should also be round-trip tested as a possible JSON object/array that can be encoded as an embedded JSCN value (common in JOSE)
* Introduce a new CBOR tag id 42 for marking values as JSCN encoded, the tag is followed by a map with integer keys containing minimally a `1` key with the JSCN value, optional additional items in the map are metadata about it such as the dictionary (`2`) and whitespace hints (`3`)
* Introduce CBOR tag 20 that is used to indicate the following value should be upper case when encoded as a string, this applies to hexadecimal tag 23 and exponent tag 4 values

### Dictionaries

* Support for custom dictionaries by using a CBOR byte string (type 2) as the lookup key, the value of which must be resolved to an above CBOR value that replaces the byte string
* When condensing JSON any UTF-8 strings (keys or values) are first checked against the active dictionary for possible replacement
* The replacement is always a CBOR byte string (type 2) of length 1, the single byte represents the index value of the key in the dictionary from 1-255, value 0 and byte lengths >1 are currently reserved
* Dictionaries are themselves identified with a unique string or integer ids, the strings must have a known mapping for apps using them and a registry will be created to assign integer ids to public well-known dictionaries
* A dictionary is represented in JSCN as an array where the first value is the dictionary id followed by all of the UTF-8 string keys, their position in the array is the byte value they are replaced with
* Dictionaries may be combined when the JSCN contains another dictionary id, any byte strings in the definition array are then replaced with the key from the given directory

### Whitespace Hints

* The "whitespace" key in a JSCN tag 42 map is an optional CBOR array value of the non-semantic whitespace present in the original JSON UTF-8 string
* The array contains only integers that reference offsets of the locations of whitespace in the original JSON string and lookup references to what whitespace contents were there
* Each offset integer is relative to the position of the previous offset such that all integers are of small values
* Any negative integer offset is a reference to a single space character (0x20) at the offset of the positive value of that integer
* All positive integer offsets are followed by another integer, positive values (0-23) reference a whitespace string in a pre-defined lookup table, negative values are the number of space characters (0x20) to repeat
* When re-inflating the whitespace the array must be applied sequentially so that each new offset matches the original JSON string position








