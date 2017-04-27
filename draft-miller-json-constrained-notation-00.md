% Title = "JSON Constrained Notation (JSCN)"
% abbrev = "JSCN"
% category = "std"
% docName = "draft"
% area = ""
% workgroup = ""
% date = 2017-04-20T00:00:00Z
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

This specification addresses the challenges of using JSON with constrained devices by providing a set of mapping rules to CBOR that are able to retain the complete semantic value of the data such that the orginal JSON string can always be identically re-created.  This constrained notation is intended to be usable directly by devices as a native data type which can always be represented as JSON when necessary for diagnostics, compatibility, and ease of integration with higher level systems.

A driving goal of this specification has been to enable the direct use of all existing JOSE standards unmodified in a constrained environment and to enable the immediate adoption of OpenID Connect as an identity management solution for the Internet of Things.

{mainmatter}

# Introduction

Constrained JSON (JSCN) is a set of rules for re-coding the JSON structures by mapping them directly to their CBOR parallels whenever possible, and then increasing the efficiency through introspection and replacement of well-known strings with compact references.

All transcoding software must operate directly on a UTF-8 JSON string whenever complete round-trip compatibly to and from JSON is required, including mapping any contained non-structural whitespace (such as with JWTs for signature validation).  If a transcoder is only operating with an already parsed JSON value (the result of `JSON.parse()` in JavaScript for instance), the round-trip can only guarantee semantic compatibility of the values as represented in that parsed context (only the JavaScript object will always match).

A significant reduction in space is also provided in JSCN when the device and application contexts can make use of built-in or shared UTF-8 string references.  These references provide a mapping of common JSON string values to an integer that used to replace the string in the resulting CBOR during re-coding.  JSON string values are also introspected for data that has a more compact CBOR type (such as base64url and hexadecimal encoding).

This specification does not currently provide for the CBOR byte strings to be canonical and only guarantees that the JSON byte strings before and after re-coding will be identical.  It also defines basic API rules for constrained software such that accessing the CBOR data values will provide a uniform view even if the underlying CBOR has encoding differences.

## Requirements Notations and Conventions

The key words "**MUST**", "**MUST NOT**", "**REQUIRED**", "**SHALL**", "**SHALL NOT**", "**SHOULD**", "**SHOULD NOT**", "**RECOMMENDED**", "**MAY**", and "**OPTIONAL**" in this document are to be interpreted as described in RFC 2119 [@!RFC2119].

## Terminology

This specification uses the following terms:

- Reference
  - Used within JSCN data to refer to well-known UTF-8 strings by using a CBOR byte string of length one, where the byte value is the reference lookup id.
- Reference Set
  - A CBOR array of UTF-8 strings that are used to replace any reference within any JSCN data, the Reference id is the array offset to the replacement string, and the first position in the array identifies that Reference Set.
- Whitespace Hints
  - A CBOR array of integers that indicate positional offsets and types of JSON whitespace strings (` `, `\n`, `\r`, and `\t`) such that when any CBOR encoded data is stringified into JSON it can also optionally be corrected to exactly match the original JSON string.

# CBOR Encoding

## Structures

Direct serialization of JSON structures to the CBOR major object/array types 4 and 5, ordering of key/value pairs in maps/objects must always be preserved.

## Simple Types

JSON `true`, `false`, and `null` are serialized to their CBOR type 7 simple values.

## Numbers 

Any JSON exponent value is encoded as a CBOR exponent (tag 4), if the contained `e` symbol is upper-case in JSON the case tag (31) must also be used.

JSON numbers should be encoded as CBOR Integers (type 0 and 1) or Floats (type 7) and then tested for compatibility by round-tripping them back to a JSON number, any remaining incompatible numbers are encoded as Bigfloats (tag 5).

## Strings

Strings are preserved as a UTF-8 string (type 3), they are always the bare escaped JSON string values and must not have been un- and re-escaped unless resticted by operating in a limited parsed context without any JSON compatibility guarantees.

### Base64 / Base16 Encoded

All JSON strings must also be round-trip tested for possible encodings (base64url, base64, and hexadecimal) by attempting to decode and re-encode them, if identical byte strings result the decoded value is tagged in CBOR with the encoding format (tags 21, 22, and 23/31).

The resulting decoded byte string must be introspected to see if it begins with a JSON structure byte of '{' or '['.  It is then round-trip tested as a possible JSON object/array to be encoded more efficiently into a CBOR data item instead of a byte string (this pattern is common in JOSE).

# Reference Sets

* The Constrained JSON tag is followed by an array who's second item identifies the Reference Set used in the data, it is either the id value or an array that defines an inline Reference Set.
* Reference Sets are themselves identified with unique integer ids that must have a known mapping for apps using them, a registry will be created to assign the integer ids to public well-known reference sets.
* A references definition is itself encoded as a JSCN array where the first value is the references id followed by all of the UTF-8 string keys, their position in the array is the byte value they are replaced with.
* Reference Sets may be combined when its JSCN definition also contains another Reference Set id, any byte strings in the definition array are then replaced with the key from the given references.
* Any JSON UTF-8 strings (keys or values) are first checked against all active references (if any) for possible replacement, any replacement is always a CBOR byte string (type 2) of length 1, the single byte represents the index value of the key in the references array from 1-255, value 0 and byte lengths >1 are currently reserved.
* When generating any JSON values from CBOR and a CBOR byte string (type 2) is encountered the single byte value must match the array offset of the active references to be used as the replacement for that byte string.

# Whitespace Hints

* Whitespace hints are an array that contains only integers which indicate offsets of the locations of whitespace in an original JSON string and lookup keys to what whitespace contents were there.
* Each offset integer is relative to the position of the previous offset such that all integers are of small values.
* Any negative integer offset indicates a single space character (0x20) at the offset of the positive value of that integer.
* All positive integer offsets are followed by another integer, positive values (0-23) indicate a whitespace string in a pre-defined lookup table, negative values are the number of space characters (0x20) to repeat.
* When adding back any whitespace to a JSON string the array must be applied sequentially so that each new offset matches the original JSON string position.

(TODO embed whitespace lookup table and examples)

# Constrained API

When the JSCN is being used directly by a software library all access to the contained data values must have both a native type and JSON-string API available so that a constrained application can choose either regardless of how the CBOR types represent the data.

For example, when a JSON string value is encoded in JSCN as a CBOR base64url tag plus byte string, the application must be able to access either the original base64url string value or the CBOR binary byte string as needed by context, but should not alter behavior based on which way the value is encoded in CBOR.

# Examples

JSON (318 bytes) to JSCN (187 bytes) with no references and whitespace preserved:

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
D4                              # tag(20)
   83                           # array(3)
      A6                        # map(6)
         63                     # text(3)
            6D6170              # "map"
         65                     # text(5)
            76616C7565          # "value"
         65                     # text(5)
            6172726179          # "array"
         84                     # array(4)
            63                  # text(3)
               6F6E65           # "one"
            63                  # text(3)
               74776F           # "two"
            65                  # text(5)
               7468726565       # "three"
            18 2A               # unsigned(42)
         64                     # text(4)
            626F6F6C            # "bool"
         F5                     # primitive(21)
         63                     # text(3)
            6E6567              # "neg"
         38 29                  # negative(41)
         66                     # text(6)
            73696D706C65        # "simple"
         83                     # array(3)
            F4                  # primitive(20)
            F6                  # primitive(22)
            60                  # text(0)
                                # ""
         64                     # text(4)
            696E7473            # "ints"
         8C                     # array(12)
            00                  # unsigned(0)
            01                  # unsigned(1)
            17                  # unsigned(23)
            18 18               # unsigned(24)
            19 00FF             # unsigned(255)
            19 0100             # unsigned(256)
            19 FFFF             # unsigned(65535)
            1A 00010000         # unsigned(65536)
            1B 00000000FFFFFFFF # unsigned(4294967295)
            1B 0000000100000000 # unsigned(4294967296)
            1B 0001000000000000 # unsigned(281474976710656)
            3B 0000FFFFFFFFFFFF # negative(281474976710655)
      00                        # unsigned(0)
      98 40                     # array(64)
         01                     # unsigned(1)
         01                     # unsigned(1)
         26                     # negative(6)
         08                     # unsigned(8)
         01                     # unsigned(1)
         28                     # negative(8)
         01                     # unsigned(1)
         02                     # unsigned(2)
         06                     # unsigned(6)
         02                     # unsigned(2)
         06                     # unsigned(6)
         02                     # unsigned(2)
         08                     # unsigned(8)
         02                     # unsigned(2)
         02                     # unsigned(2)
         01                     # unsigned(1)
         02                     # unsigned(2)
         01                     # unsigned(1)
         27                     # negative(7)
         05                     # unsigned(5)
         01                     # unsigned(1)
         26                     # negative(6)
         04                     # unsigned(4)
         01                     # unsigned(1)
         29                     # negative(9)
         01                     # unsigned(1)
         02                     # unsigned(2)
         06                     # unsigned(6)
         02                     # unsigned(2)
         05                     # unsigned(5)
         02                     # unsigned(2)
         02                     # unsigned(2)
         01                     # unsigned(1)
         02                     # unsigned(2)
         01                     # unsigned(1)
         27                     # negative(7)
         01                     # unsigned(1)
         02                     # unsigned(2)
         02                     # unsigned(2)
         02                     # unsigned(2)
         02                     # unsigned(2)
         02                     # unsigned(2)
         03                     # unsigned(3)
         02                     # unsigned(2)
         03                     # unsigned(3)
         02                     # unsigned(2)
         04                     # unsigned(4)
         02                     # unsigned(2)
         04                     # unsigned(4)
         02                     # unsigned(2)
         06                     # unsigned(6)
         02                     # unsigned(2)
         06                     # unsigned(6)
         02                     # unsigned(2)
         0B                     # unsigned(11)
         02                     # unsigned(2)
         0B                     # unsigned(11)
         02                     # unsigned(2)
         10                     # unsigned(16)
         02                     # unsigned(2)
         10                     # unsigned(16)
         01                     # unsigned(1)
         01                     # unsigned(1)
         00                     # unsigned(0)
```

With no whitespace and using a Reference Set of `[1,"map","value","array","one","two","three","bool","neg","simple","ints"]` (90 bytes):
``` ascii-art
D4                              # tag(20)
   82                           # array(2)
      A6                        # map(6)
         41                     # bytes(1)
            01                  # "\x01"
         41                     # bytes(1)
            02                  # "\x02"
         41                     # bytes(1)
            03                  # "\x03"
         84                     # array(4)
            41                  # bytes(1)
               04               # "\x04"
            41                  # bytes(1)
               05               # "\x05"
            41                  # bytes(1)
               06               # "\x06"
            18 2A               # unsigned(42)
         41                     # bytes(1)
            07                  # "\a"
         F5                     # primitive(21)
         41                     # bytes(1)
            08                  # "\b"
         38 29                  # negative(41)
         41                     # bytes(1)
            09                  # "\t"
         83                     # array(3)
            F4                  # primitive(20)
            F6                  # primitive(22)
            60                  # text(0)
                                # ""
         41                     # bytes(1)
            0A                  # "\n"
         8C                     # array(12)
            00                  # unsigned(0)
            01                  # unsigned(1)
            17                  # unsigned(23)
            18 18               # unsigned(24)
            19 00FF             # unsigned(255)
            19 0100             # unsigned(256)
            19 FFFF             # unsigned(65535)
            1A 00010000         # unsigned(65536)
            1B 00000000FFFFFFFF # unsigned(4294967295)
            1B 0000000100000000 # unsigned(4294967296)
            1B 0001000000000000 # unsigned(281474976710656)
            3B 0000FFFFFFFFFFFF # negative(281474976710655)
      01                        # unsigned(1)
```

JWT (149 bytes) `eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiYWRtaW4iOnRydWV9.TJVA95OrM7E2cBab30RMHrHDcEfxjoYZgeFONFh7HgQ` converted to JSON (191):
``` json
{"protected":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9","payload":"eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiYWRtaW4iOnRydWV9","signature":"TJVA95OrM7E2cBab30RMHrHDcEfxjoYZgeFONFh7HgQ"}
```

JSCN (80 bytes) using a Reference Set of `[1,"payload","signature","protected","alg","HS256","sub","name","admin"]`:
``` ascii-art
D4                                      # tag(20)
   82                                   # array(2)
      A3                                # map(3)
         41                             # bytes(1)
            03                          # "\x03"
         D5                             # tag(21)
            A2                          # map(2)
               41                       # bytes(1)
                  04                    # "\x04"
               41                       # bytes(1)
                  05                    # "\x05"
               41                       # bytes(1)
                  06                    # "\x06"
               41                       # bytes(1)
                  07                    # "\a"
         41                             # bytes(1)
            01                          # "\x01"
         D5                             # tag(21)
            A3                          # map(3)
               41                       # bytes(1)
                  08                    # "\b"
               D7                       # tag(23)
                  45                    # bytes(5)
                     1234567890         # "\x124Vx\x90"
               41                       # bytes(1)
                  09                    # "\t"
               68                       # text(8)
                  4A6F686E20446F65      # "John Doe"
               41                       # bytes(1)
                  0A                    # "\n"
               F5                       # primitive(21)
         41                             # bytes(1)
            02                          # "\x02"
         D5                             # tag(21)
            58 20                       # bytes(32)
               4C9540F793AB33B13670169BDF444C1EB1C37047F18E861981E14E34587B1E04 # "L\x95@\xF7\x93\xAB3\xB16p\x16\x9B\xDFDL\x1E\xB1\xC3pG\xF1\x8E\x86\x19\x81\xE1N4X{\x1E\x04"
      01                                # unsigned(1)
```

# IANA Considerations

The IANA is requested to assign the following tags from the "CBOR Tags" registry defined in RFC 7049 [@!RFC7049]:

* Assign the tag "Constrained JSON" in the 1 to 23 value range (one byte in length when encoded).

* Assign the tag "Upper Case Modifier" in the 24 to 255 value range (two bytes in length when encoded).

The tags to be assigned are described below.

## CBOR Tag Registrations

```
Tag             20 (Constrained JSON)
Data Item       array
Semantics       First value in the array is always a constrained JSON data
                item encoded using JSCN, optionally followed by an integer
                or array identifying any embedded references, and then an
                optional array of whitespace hints if any.
Reference       http://quartzjer.github.io/JSCN
Contact         Jeremie Miller <jeremie.miller@gmail.com>

Tag             31 (Upper Case Modifier)
Data Item       multiple
Semantics       Indicates that the data item following contains values
                where the upper case is semantically important when 
                interpreted in any UTF-8 string context.
Reference       http://quartzjer.github.io/JSCN
Contact         Jeremie Miller <jeremie.miller@gmail.com>
```


# Security Considerations

TODO

{backmatter}

# Acknowledgements

The author wishes to thank you in advance.

# Notices

TODO
