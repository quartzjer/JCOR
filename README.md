# JSCN - JSON Constrained Notation

## Scope

A subset of CBOR that symmetrically mirrors any JSON value into a compact serialization for constrained nodes and networks.

* optimizes for compact representations wherever possible without breaking symmetry
* CBOR->JSON->CBOR->JSON must always result in identical byte/UTF-8 strings
* non-semantic whitespace is discarded during serialization
  * the initial JSON->CBOR->JSON may result in different UTF-8 strings
* serialization is optional in any security context where the original JSON UTF-8 string is used
  * serializers must always perform a full round-trip and compare to validate if the CBOR representation is safe
* the api used by a constrained node to access JSCN should be able to return either the JSON UTF-8 or native CBOR type for any value
  * the api should also accept JSON and internally generate the CBOR to provide a uniform interface (useful when a security context may contain either format)
* constrained applications built using JSCN natively to efficiently exchange binary values have the benefit of representing all data in JSON whenever necessary
* only the JSON encoding is canonical, the CBOR encoded byte string may vary based on the serializer implementation capabilities

## Proposal

* Direct serialization of JSON structures to CBOR major types objects and arrays (types 4 and 5)
* `true`, `false`, and `null` are serialized to their simple values (type 7)
* JSON numbers/strings must be round-trip tested to guarantee symmetry with the encoding chosen
* Numbers should be expressed as Integers (type 0 and 1) or Floats (type 7) when round-trip compatible
  * Remaining numbers are encoded as Bigfloats (tag 5) or exponents (tag 4)
* Strings should be round-trip tested for possible HEX encoding (tag 23), base64url (tag 21), and base64 (tag 22)
  * Remaining strings are preserved as a UTF-8 string (type 3)
* Tag 20 is used to indicate the following value should be upper case in JSON for both HEX (tag 23) and exponents (tag 4's e/E)
* Ordering of key/value pairs in maps/objects must always be preserved
* Support for custom dictionaries by using the CBOR byte string (type 2) as the lookup key, the value of which must be resolved to an above CBOR value that replaces the byte string

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











