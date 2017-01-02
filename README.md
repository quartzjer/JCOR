# CJS - Constrained JSON Serialization

## Scope

Enable any JSON value to be serialized using CBOR such that it can be compatibly de-serialized back to JSON.

* Serialization should optimize for compact representations wherever possible
* Whitespace is not preserved so JSON->CBOR->JSON may result in different UTF8 strings, whereas JSON without any structural whitespace must result in identical UTF8 strings.
* CBOR->JSON->CBOR must always result in identical byte strings

## Proposal

* Restrict CBOR usage to only major types 0 through 5 (byte and UTF-8 strings, integers, map and array), two tags, and the simple values `true`, `false`, and `null`
* Tag 4 is used to serialize all float and exponent numbers
* Tag 21 is used to serialize base64url values (commonly used in JOSE)
* Tag 24 with a byte string item is used to serialize a raw JSON object/array where the exact bytes must be kept intact (for crypto)
* Include support for custom dictionaries by using the byte string type as the lookup key, the value of which must be a UTF-8 string that replaces the byte string
* All JSON numbers that are integers must be converted to a CBOR integer, all other numbers (exponents and fractions) must be preserved using Tag 24
* Ordering of key/value pairs in maps/objects must always be preserved


```
         ┌──────────┐                         ┌──────────┐
         │   keys   │                         │   vals   │
┌────────┴──────────┴────────┐       ┌────────┴──────────┴────────┐
│                            │       │                            │
│     ┌─────────────────┐    │       │     ┌─────────────────┐    │
│     │  byte - table   │──┐ │       │     │  byte - table   │──┐ │
│     └─────────────────┘  │ │       │     └─────────────────┘  │ │
│                          │ │       │                          │ │
│     ┌─────────────────┐  │ │       │     ┌─────────────────┐  │ │
│     │  utf8 - string  │◀─┘ │       │     │  utf8 - string  │◀─┘ │
│     └─────────────────┘    │       │     └─────────────────┘    │
│                            │       │     ┌─────────────────┐    │
│                            │       │     │  ±int - number  │    │
│                            │       │     └─────────────────┘    │
│                            │       │     ┌─────────────────┐    │
│                            │       │     │      {map}      │    │
│                            │       │     └─────────────────┘    │
│                            │       │     ┌─────────────────┐    │
│                            │       │     │     [array]     │    │
│                            │       │     └─────────────────┘    │
│                            │       │     ┌─────────────────┐    │
│                            │       │     │ tag JSON - utf8 │    │
│                            │       │     └─────────────────┘    │
│                            │       │     ┌─────────────────┐    │
│                            │       │     │ tag b64u + byte │    │
│                            │       │     └─────────────────┘    │
│                            │       │                            │
└────────────────────────────┘       └────────────────────────────┘



        ┌──────────┐
        │  array   │
┌───────┴──────────┴─────────────────────────────────────────────────┐
│                                                                    │
│                                                                    │
│    ┌──────────┐    ┌──────────┐    ┌──────────┐                    │
│    │   val    │    │   val    │    │   val    │      ...           │
│    └──────────┘    └──────────┘    └──────────┘                    │
│                                                                    │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```