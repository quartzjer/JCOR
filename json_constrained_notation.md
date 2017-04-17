% Title = "JSON Constrained Notation"
% abbrev = "jscn"
% category = "std"
% docName = "json-constrained-notation"
% area = ""
% workgroup = ""
% date = 2017-04-07T00:00:00Z
% ipr = "none"
%
% keyword = ["JSON", "CBOR", "constrained", "JOSE", "JWT"]
%
% [pi]
% private = "Draft"
% compact = "yes"
% subcompact = "no"
% tocdepth = "5"
% topblock = "yes"
% comments = "no"
% iprnotified = "no"
%
% [[author]]
% initials="J."
% surname="Miller"
% fullname="Jeremie Miller"
%   [author.address]
%   email = "jeremie@jabber.org"
<!--
  NOTE:  This Markdown file and any generated XML file is input used to produce the authoritative copy of an IETF specification.  The authoritative copy is the HTML output.  This XML source file is not authoritative.  The statement ipr="none" is present only to satisfy the document compilation tool and is not indicative of the IPR status of this specification.  The IPR for this specification is described in the "Notices" section.  This is a public IETF document and not a private document, as the private="..." declaration could be taken to indicate.
-->
.# Abstract

The adoption of JSON is widespread in all traditional networking and software environments, but there has been more limited use in embedded and constrained environments due to the always-minimized storage and network capacities inherent in low-cost and low-power devices.

This specification addresses the challenges of using JSON with constrained devices by providing a set of simple mapping rules to CBOR that are able to retain the complete semantic value of the data such that the orginal JSON string can always be identically re-created.  This constrained notation is intended to be usable directly by devices as a native data type which can always be represented as JSON when necessary for diagnostics, compatibility, and ease of integration with higher level systems.

A driving goal of this specification has been to enable the direct use of all existing JOSE standards unmodified in a constrained environment and to enable the immediate adoption of OpenID Connect as an Identity Management standard for the IoT.

{mainmatter}

# Introduction

JSCN is a subset of CBOR, the rules for re-coding the JSON structures all map directly to their CBOR parallels whenever possible.  Only when JSON string values can be introspected to contain data that has a more compact CBOR type (such as base64url and hexadecimal encoding) is additional mapping performed when re-coding the string value.

All JSCN transcoder software must operate directly on a UTF-8 JSON string whenever round-trip compatibly to and from JSON is required including any contained non-structural whitespace (such as with JWTs for signature validation).  If a transcoder is only operating with an already parsed JSON value (the result of `JSON.parse()` in JavaScript for instance), the round-trip can only guarantee semantic compatibility of the values as represented in that parsed context.

A significant reduction in space is also provided in JSCN when the device and application contexts can make use of built-in or shared dictionaries.  These dictionaries provide a mapping of common JSON string values to an integer that used instead in the resulting CBOR during re-coding.

The CBOR resulting from a re-coding of JSON to JSCN may have slight differences based on the capabilities of the transcoder software, this specification does not currently provide for the JSCN byte strings to be canonical and only guarantees that the JSON byte strings before and after re-coding will be identical as well as the view any constrained device has of the data values will be uniform.

## Requirements Notations and Conventions

The key words "**MUST**", "**MUST NOT**", "**REQUIRED**", "**SHALL**", "**SHALL NOT**", "**SHOULD**", "**SHOULD NOT**", "**RECOMMENDED**", "**MAY**", and "**OPTIONAL**" in this document are to be interpreted as described in RFC 2119 [@!RFC2119].

## Terminology

This specification defines the following terms:

- JSCN Tag
  - CBOR tag id 42 for marking values as JSCN encoded, the tag is followed by a map with integer keys containing minimally a `1` key with the JSCN value, optional additional items in the map are metadata about it such as the dictionary (`2`) and whitespace hints (`3`).
- Case Tag
  - CBOR tag id 20 that is used to indicate the following value should be upper case when encoded as a string, this applies to hexadecimal tag 23 and exponent tag 4 values.
- Dictionary
- Whitespace Hints

# CBOR Encoding

## Structures

Direct serialization of JSON structures to the CBOR major object/array types 4 and 5, ordering of key/value pairs in maps/objects must always be preserved.

## Simple Types

JSON `true`, `false`, and `null` are serialized to their CBOR type 7 simple values.

## Numbers 

Any JSON exponent value is encoded as a CBOR exponent (tag 4), if the contained `e` symbol is upper-case in JSON the case tag (20) must also be used.

JSON numbers should be encoded as CBOR Integers (type 0 and 1) or Floats (type 7) and then tested for compatibility by round-tripping them back to a JSON number, any remaining incompatible numbers are encoded as Bigfloats (tag 5).

## Strings

Strings are preserved as a UTF-8 string (type 3), they are always the bare escaped JSON string values and must not have been un- and re-escaped unless resticted by operating in a limited parsed context without any JSON compatibility guarantees.

### Base64 / Base16 Encoded

All JSON strings must also be round-trip tested for possible encodings (base64url, base64, and hexadecimal) by attempting to decode and re-encode them, if identical byte strings result the decoded value is tagged in CBOR with the encoding format (tags 21, 22, and 20/23).

The resulting decoded byte string must be introspected if it begins with a JSON structure byte of '{' or '['.  It must also then be round-trip tested as a possible JSON object/array that can be encoded as an embedded JSCN value (this pattern is common in JOSE).

# Dictionaries

* Support for custom dictionaries by using a CBOR byte string (type 2) as the lookup key, the value of which must be resolved to an above CBOR value that replaces the byte string
* When condensing JSON any UTF-8 strings (keys or values) are first checked against the active dictionary for possible replacement
* The replacement is always a CBOR byte string (type 2) of length 1, the single byte represents the index value of the key in the dictionary from 1-255, value 0 and byte lengths >1 are currently reserved
* Dictionaries are themselves identified with a unique string or integer ids, the strings must have a known mapping for apps using them and a registry will be created to assign integer ids to public well-known dictionaries
* A dictionary is represented in JSCN as an array where the first value is the dictionary id followed by all of the UTF-8 string keys, their position in the array is the byte value they are replaced with
* Dictionaries may be combined when the JSCN contains another dictionary id, any byte strings in the definition array are then replaced with the key from the given directory

# Whitespace Hints

* The "whitespace" key in a JSCN tag 42 map is an optional CBOR array value of the non-semantic whitespace present in the original JSON UTF-8 string
* The array contains only integers that reference offsets of the locations of whitespace in the original JSON string and lookup references to what whitespace contents were there
* Each offset integer is relative to the position of the previous offset such that all integers are of small values
* Any negative integer offset is a reference to a single space character (0x20) at the offset of the positive value of that integer
* All positive integer offsets are followed by another integer, positive values (0-23) reference a whitespace string in a pre-defined lookup table, negative values are the number of space characters (0x20) to repeat
* When re-inflating the whitespace the array must be applied sequentially so that each new offset matches the original JSON string position

(TODO embed whitespace lookup table)

# Constrained API

When the JSCN is being used directly by a software library all access to the contained data values must have both a native type and JSON-string API available so that a constrained application can choose either regardless of how the CBOR types represent the data.

For example, when a JSON string value is encoded in JSCN as a CBOR base64 tag plus byte string, the application must be able to access either the original string value or the CBOR binary byte string as needed by context.

# Examples

JSON (318 bytes) to JSCN (189 bytes) with no dictionary and whitespace preserved:
```json
{
  "map": "value",
  "array": [
    "one",
    "two",
    "three",
    42
  ],
  "bool": true,
  "neg": -42,
  "simple": [
    false,
    null,
    ""
  ],
  "ints": [
    0,
    1,
    23,
    24,
    255,
    256,
    65535,
    65536,
    4294967295,
    4294967296,
    281474976710656,
    -281474976710656
  ]
}
```

``` ascii-art
  Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 	
00000000: D8 2A A2 01 A6 63 6D 61 70 65 76 61 6C 75 65 65    X*".&cmapevaluee
00000010: 61 72 72 61 79 84 63 6F 6E 65 63 74 77 6F 65 74    array.conectwoet
00000020: 68 72 65 65 18 2A 64 62 6F 6F 6C F5 63 6E 65 67    hree.*dboolucneg
00000030: 38 29 66 73 69 6D 70 6C 65 83 F4 F6 60 64 69 6E    8)fsimple.tv`din
00000040: 74 73 8C 00 01 17 18 18 19 00 FF 19 01 00 19 FF    ts..............
00000050: FF 1A 00 01 00 00 1B 00 00 00 00 FF FF FF FF 1B    ................
00000060: 00 00 00 01 00 00 00 00 1B 00 01 00 00 00 00 00    ................
00000070: 00 3B 00 00 FF FF FF FF FF FF 03 9F 01 01 26 08    .;............&.
00000080: 01 28 01 02 06 02 06 02 08 02 02 01 02 01 27 05    .(............'.
00000090: 01 26 04 01 29 01 02 06 02 05 02 02 01 02 01 27    .&..)..........'
000000a0: 01 02 02 02 02 02 03 02 03 02 04 02 04 02 06 02    ................
000000b0: 06 02 0B 02 0B 02 10 02 10 01 01 00 FF             .............
```

With no whitespace and using a dictionary of `[1,"map","value","array","one","two","three","bool","neg","simple","ints"]` (93 bytes):
``` ascii-art
  Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 	
00000000: D8 2A A2 02 01 01 A6 41 01 41 02 41 03 84 41 04    X*"...&A.A.A..A.
00000010: 41 05 41 06 18 2A 41 07 F5 41 08 38 29 41 09 83    A.A..*A.uA.8)A..
00000020: F4 F6 60 41 0A 8C 00 01 17 18 18 19 00 FF 19 01    tv`A............
00000030: 00 19 FF FF 1A 00 01 00 00 1B 00 00 00 00 FF FF    ................
00000040: FF FF 1B 00 00 00 01 00 00 00 00 1B 00 01 00 00    ................
00000050: 00 00 00 00 3B 00 00 FF FF FF FF FF FF             ....;........
```

# Security Concerns

TODO

{backmatter}

# Acknowledgements

The author wishes to thank you.

# Notices

TODO
