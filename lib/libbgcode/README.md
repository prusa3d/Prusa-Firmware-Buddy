# libbgcode

Prusa Block &amp; Binary G-code reader / writer / converter

# Description

A new G-code file format featuring the following improvements over the legacy G-code:
1) Block structure with distinct blocks for metadata vs. G-code
2) Faster navigation
3) Coding & compression for smaller file size
4) Checksum for data validity
5) Extensivity through new (custom) blocks. For example, a file signature block may be welcome by corporate customers.

**libbgcode** library is split into three API:
* [core](#core-api)
* [binarize](#binarize-api)
* [convert](#convert-api)

### core API

**core api** contains the basic definitions and functionality which allow to read a G-code file in binary format as defined into [SPECIFICATIONS](doc/specifications.md).

See [src/LibBGCode/core/core.hpp](src/LibBGCode/core/core.hpp)

### binarize API

**binarize api** contains the definitions and functionality which allow to write a G-code file in binary format as defined into [SPECIFICATIONS](doc/specifications.md).

See [src/LibBGCode/binarize/binarize.hpp](src/LibBGCode/binarize/binarize.hpp)

### convert API

**convert api** contains the functionality which allow to convert G-code files to/from binary format as defined into [SPECIFICATIONS](doc/specifications.md).

See [src/LibBGCode/convert/convert.hpp](src/LibBGCode/convert/convert.hpp)

# Specifications

See [SPECIFICATIONS](doc/specifications.md) for file format specifications.

# Building

See [BUILDING](doc/building.md) for building instructions.

# Command line application

See [BGCODE](doc/bgcode.md) for instructions.

# References

We would like to thank Scott Vokes (@atomicobject) for his work on heatshrink and
Scott Mudge (@scottmudge) for developing and maintaining MeatPack.