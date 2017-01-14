# CJS - Constrained JSON Serialization

## Scope

Enable any JSON value to be serialized using CBOR such that it can be compatibly de-serialized back to JSON.

* Serialization should optimize for compact representations wherever possible
* Whitespace is not preserved so JSON->CBOR->JSON may result in different UTF8 strings, whereas JSON without any structural whitespace must result in identical UTF8 strings.
* CBOR->JSON->CBOR must always result in identical byte strings

## Proposal

* Direct serialization of JSON values to CBOR major types for UTF-8 strings (type 3), integers (types 0 and 1), objects and arrays (types 4 and 5)
* `true`, `false`, and `null` are serialized to their simple values (type 7)
* Tag 4 is used to serialize all JSON float and exponent numbers
* Tag 21 may used to serialize any detected base64url encoded JSON strings (commonly used in JOSE)
* Tag 24 with a byte string item is used to serialize a raw JSON object/array where the exact bytes including whitespace must be kept intact (for crypto)
* Include support for custom dictionaries by using the CBOR byte string (type 2) as the lookup key, the value of which must be a UTF-8 string that replaces the byte string
* All JSON numbers that are integers must be converted to a CBOR integer, all other numbers (exponents and fractions) must be preserved using Tag 24
* Ordering of key/value pairs in maps/objects must always be preserved


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

        ┏━━━━━━━━━━┓
        ┃  array   ┃
┏━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                                                                    ┃
┃                                                                    ┃
┃    ╔══════════╗    ╔══════════╗    ╔══════════╗                    ┃
┃    ║   val    ║    ║   val    ║    ║   val    ║      ...           ┃
┃    ╚══════════╝    ╚══════════╝    ╚══════════╝                    ┃
┃                                                                    ┃
┃                                                                    ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛


        ┏━━━━━━━━━━┓
        ┃   map    ┃
┏━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━━━━━━━━┓
┃                                    ┃
┃                                    ┃
┃    ╔══════════╗    ╔══════════╗    ┃
┃    ║   key    ║    ║   val    ║    ┃
┃    ╚══════════╝    ╚══════════╝    ┃
┃                                    ┃
┃    ╔══════════╗    ╔══════════╗    ┃
┃    ║   key    ║    ║   val    ║    ┃
┃    ╚══════════╝    ╚══════════╝    ┃
┃                                    ┃
┃                                    ┃
┃        ...                         ┃
┃                                    ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

## Dictionaries

braindump:

* public dictionary ids are only uint's, private/custom only negative ints
* 0 is never valid except as a key, value always the id of dictionary used in given map
* 1-23 are standards-based dictionaries only
* dictionary definition is an array containing the id as the first element and followed by the string values associated with their position in the array: [22,"one","two",...]
* current dictionary is always inherited in sub-objects until another dictionary is set











