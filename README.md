# JSCN - JSON Constrained Notation

## Scope

A subset of CBOR that symmetrically mirrors any JSON value into a compact serialization for constrained nodes and networks.

* optimizes for compact representations wherever possible without breaking symmetry
* CBOR->JSON->CBOR->JSON must always result in identical byte/UTF-8 strings
* non-semantic whitespace is discarded during serialization
  * the initial JSON->CBOR->JSON may result in different UTF-8 strings
* serialization is optional in any security context where the original JSON UTF-8 string is used
  * serializers must always perform a full round-trip and compare to validate if the CBOR representation is safe
* the api used by a constrained node should be able to return either the JSON UTF-8 or native CBOR type for any value
* constrained applications should natively use the CBOR representation to efficiently exchange binary values that can still be represented in JSON whenever necessary

## Proposal

* Direct serialization of JSON values to CBOR major types for UTF-8 strings (type 3), integers (types 0 and 1), objects and arrays (types 4 and 5)
* `true`, `false`, and `null` are serialized to their simple values (type 7)
* Tag 4 is used to serialize all JSON float and exponent numbers
* Tag 21 is used to serialize any detected base64url encoded JSON strings (commonly used in JOSE)
* Tag 23 is used to serialize any detected lower-case HEX encoded JSON strings
* Ordering of key/value pairs in maps/objects must always be preserved
* Support for custom dictionaries by using the CBOR byte string (type 2) as the key, the value of which must be resolved to a UTF-8 string that replaces the byte string


```
         ╔══════════╗                         ╔══════════╗
         ║   key    ║                         ║   val    ║
╔════════╩══════════╩════════╗       ╔════════╩══════════╩════════╗
║                            ║       ║                            ║
║ ┌─────────┐    ┌─────────┐ ║       ║ ┌─────────┐    ┌─────────┐ ║
║ │ Type 3  ├───▶│ string  │ ║       ║ │ Type 3  ├───▶│ string  │ ║
║ │ (UTF-8) │ ┌─▶│         │ ║       ║ │ (UTF-8) │ ┌─▶│         │ ║
║ └─────────┘ │  └─────────┘ ║       ║ └─────────┘ │  └─────────┘ ║
║         lookup()           ║       ║         lookup()  ▲        ║
║ ┌─────────┐ │              ║       ║ ┌─────────┐ │     │        ║
║ │ Type 2  ├─┘              ║       ║ │ Type 2  ├─┘     │        ║
║ │ (byte)  │                ║       ║ │ (byte)  │       │        ║
║ └─────────┘                ║       ║ └─────────┘  base64url()   ║
║                            ║       ║ ┌─────────┐       │        ║
║                            ║       ║ │Tag 21:2 │───────┘        ║
║                            ║       ║ │ (byte)  │                ║
║                            ║       ║ └─────────┘                ║
║                            ║       ║ ┌─────────┐    ┌─────────┐ ║
║                            ║       ║ │Type 0,1 ├───▶│ number  │ ║
║                            ║       ║ │ (±int)  │ ┌─▶│         │ ║
║                            ║       ║ └─────────┘ │  └─────────┘ ║
║                            ║       ║ ┌─────────┐ │              ║
║                            ║       ║ │  Tag 4  ├─┘              ║
║                            ║       ║ │(fra/exp)│                ║
║                            ║       ║ └─────────┘                ║
║                            ║       ║ ┌─────────┐    ┌─────────┐ ║
║                            ║       ║ │ Type 4  ├───▶│  array  │ ║
╚════════════════════════════╝       ║ │ (array) │    │         │ ║
                                     ║ └─────────┘    └─────────┘ ║
                                     ║ ┌─────────┐    ┌─────────┐ ║
                                     ║ │ Type 5  ├───▶│ object  │ ║
                                     ║ │  (map)  │    │         │ ║
                                     ║ └─────────┘    └─────────┘ ║
                                     ║ ┌─────────┐    ┌─────────┐ ║
                                     ║ │Type 7:+ ├───▶│boolean, │ ║
                                     ║ │(simple) │    │  null   │ ║
                                     ║ └─────────┘    └─────────┘ ║
                                     ║ ┌─────────┐    ┌─────────┐ ║
                                     ║ │Tag 24:2 ├───▶│raw bytes│ ║
                                     ║ │(direct) │    │         │ ║
                                     ║ └─────────┘    └─────────┘ ║
                                     ╚════════════════════════════╝

```

## Dictionaries

braindump:

* public dictionary ids are only unsigned int's, private/custom only negative ints
* byte lookup of len 0 is only valid as a key with an integer value of the dictionary id for the map it is a member of
* byte lookup of len 1 is used as the index value (0-255) to resolve the utf8 string replacement
* byte lengths >1 and index value 0 are reserved
* dictionary definition is an array containing the id as the first element and followed by the string values associated with their position in the array: [22,"one","two",...]
* current dictionary is always inherited by descendent objects that do not have their own dictionary id 
* tools can internally support one level of dictionary chaining?
  * replace second number with its values: [-3,22,"four","five",...]











