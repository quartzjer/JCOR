# JSCN - JSON Constrained Notation

## Scope

A subset of CBOR that perfectly mirrors any JSON string into a highly condensed notation for constrained nodes and networks.

* Round-trip from JSON->CBOR->JSON must always result in indentical UTF-8 strings
* Optimizes for compact representations wherever possible without breaking symmetry
* The CBOR notation must be easily usable by constrained applications as a native data encoding for binary values such that all data can always be safely expanded to JSON if necessary
* Only the JSON encoding is canonical, the CBOR encoder implementations must produce identical semantic structures but the actual byte string may have slight variations
* JSCN encoders must operate directly on the UTF-8 JSON string when round-trip symmetry is required, if operating with a parsed JSON value the round-trip can only guarantee semantic compatibility within that parsed context

## Proposal

* Direct serialization of JSON structures to the CBOR major object/array types 4 and 5, ordering of key/value pairs in maps/objects must always be preserved
* JSON `true`, `false`, and `null` are serialized to their CBOR type 7 simple values
* Any JSON exponent value is encoded as a CBOR exponent (tag 4), if the contained `e` symbol is upper-case in JSON the case tag (20) must also be used
* JSON numbers should be encoded as CBOR Integers (type 0 and 1) or Floats (type 7) and then tested for compatibility by round-tripping them back to a JSON number, any remaining incompatible numbers are encoded as Bigfloats (tag 5)
* All JSON strings must be round-trip tested for possible encodings (base64url, base64, and hexadecimal) by attempting to decode and re-encode them, if identical UTF-8 strings result the decoded value is tagged in CBOR with the encoding format (tags 21, 22, and 20/23), any remaining strings are preserved as a UTF-8 string (type 3)
* Introduce a new CBOR tag id 42 for marking values as JSCN encoded, the tag is followed by an array with the first item being the JSCN byte string and optional additional items in the array containing metadata about it
* Introduce CBOR tag 20 that is used to indicate the following value should be upper case when encoded as a string, this applies to hexadecimal tag 23 and exponent tag 4 values
* Any JSON string that was safely decoded from hex/base64url/base64 must be then round-trip tested as a self-contained JSON value and the JSCN (tag 42) used instead of the decoded byte string

### Dictionaries

* Support for custom dictionaries by using a CBOR byte string (type 2) as the lookup key, the value of which must be resolved to an above CBOR value that replaces the byte string
* When condensing JSON any UTF-8 strings (keys or values) are first checked against the active dictionary for possible replacement
* The replacement is always a CBOR byte string (type 2) of length 1, the single byte represents the index value of the key in the dictionary from 1-255, value 0 and byte lengths >1 are currently reserved
* Dictionaries are themselves identified with a unique integer id, public well-known dictionaries should use positive integers and private / application-specific dictionaries should use negative integers, id 0 is reserverd
* A dictionary is represented in JSCN as the condensed form of an array where the first value is the dictionary id followed by all of the UTF-8 string keys, their position in the array is the byte value they are replaced with
* A single level of dictionary inheritance is represented by the second value in the array being the integer id of another dictionary, who's contained keys are then inserted in place of the id retaining their positional values in the resulting combined larger dictionary
* The second value in the JSCN tag array is the optional dictionary id to be applied for it
* When the JSON is known to contain string values that are also encoded JSON values (such as base64url encoded headers, claims, etc in JOSE) an additional map may be provided to a dictionary that contains the keys for which the values are the id of the dictionary to be applied when those keys match the key in a JSON object and its value is being test-condensed to JSCN, the given dictionary is then saved in any resulting JSCN tag 42 array

### Whitespace Hints

* The third item in a JSCN tag 42 array is an optional CBOR map of the non-semantic whitespace present in the original JSON UTF-8 string
* The map keys are a CBOR unsigned integer type 0 that is the byte position in the original string of one or more whitespace characters
* The map values are a CBOR UTF-8 string of one or more whitespace characters to be inserted at that position
* When re-inflating the whitespace they must be applied in incremental positional order








