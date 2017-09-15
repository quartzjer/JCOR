# JCoR - JSON Constrained Representation

This repo contains the [Internet-Draft](https://datatracker.ietf.org/doc/draft-miller-json-constrained-representation/) and a working implementation in C.

## Draft Spec

This spec enables any existing JSON or JWT to be easily re-coded into minimized CBOR for constrained networks and devices with zero loss of data or cryptographic integrity.

The goal is to continue improving the draft with feedback until it can be formally accepted into an IETF working group on the standards track.

## Code/Tools

The included implementation is permissively [MIT icensed](http://quartzjer.github.io/JSCN/LICENSE) and small / optimized for embedded usage, improvements welcome.

The next step planned is to add/create an embedded-friendly API for accessing the constrained JSON directly.
