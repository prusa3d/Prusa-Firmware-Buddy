# Specifications
The new binarized G-code file consists of a file header followed by an ordered succession of blocks, in the following sequence:
1. File Header
2. File Metadata Block (optional)
3. Printer Metadata Block
4. Thumbnails Blocks (optional)
5. Print Metadata Block
6. Slicer Metadata Block
7. G-code Blocks

All of the multi-byte integers are encoded in little-endian byte ordering.

## File header

The file header contains the following data:

|               | type     | size    | description                        |
| ------------- | -------- | ------- | ---------------------------------- |
| Magic Number  | uint32_t | 4 bytes | GCDE                               |
| Version       | uint32_t | 4 bytes | Version of the G-code binarization |
| Checksum type | uint16_t | 2 bytes | Algorithm used for checksum        |

The size in bytes of the file header is 10.

Current value for `Version` is **1**

Possible values for `Checksum type` are:
```
0 = None
1 = CRC32
```

## Blocks

All blocks have the following structure:
* [Block header](#block-header)
* [Block parameters](#block-parameters)
* [Block data](#block-data)
* [Block checksum (optional)](#block-checksum)

### Block header

The block header is the same for all blocks.
It is defined as:

|                   | type     | size    | description                        |
| ----------------- | -------- | ------- | ---------------------------------- |
| Type              | uint16_t | 2 bytes | Block type                         |
| Compression       | uint16_t | 2 bytes | Compression algorithm              |
| Uncompressed size | uint32_t | 4 bytes | Size of the data when uncompressed |
| Compressed size   | uint32_t | 4 bytes | Size of the data when compressed   |

The size in bytes of the block header is **8** when `Compression` = **0** and **12** in all other cases.

Possible values for `Type` are:
```
0 = File Metadata Block
1 = GCode Block
2 = Slicer Metadata Block
3 = Printer Metadata Block
4 = Print Metadata Block
5 = Thumbnail Block
```

Possible values for `Compression` are:
```
0 = No compression
1 = Deflate algorithm
2 = Heatshrink algorithm with window size 11 and lookahead size 4
3 = Heatshrink algorithm with window size 12 and lookahead size 4
```

### Block parameters
Block parameters are used to let readers be able to interpret the block data.

Each block type may define its own parameters, so the size in bytes may vary.

### Block data
Block data content is described by the block paramenters.

The size in bytes of the block data is defined in the block header.
For `Compression` = **0** it is `Uncompressed size`, otherwise it is `Compressed size`.

### Block checksum
Block checksum is present when the `Checksum type` in the file header is different from **0**.

The size in bytes depends on the selected algorithm. For CRC32 it is **4**.

### Block types

  * [File metadata](#file-metadata)
  * [Printer metadata](#printer-metadata)
  * [Thumbnail](#thumbnail)
  * [Print metadata](#print-metadata)
  * [Slicer metadata](#slicer-metadata)
  * [GCode](#gcode)

### File metadata
Table of key-value pairs of generic metadata, such as producer (software), etc.

#### Parameters
|          | type     | size    | description   |
| -------- | -------- | ------- | ------------- |
| Encoding | uint16_t | 2 bytes | Encoding type |

Possible values for `Encoding` are:
```
0 = INI encoding
```

### Printer metadata
Table of key-value pairs of metadata consumed by printer, such as printer model, nozzle diameter etc.

#### Parameters
|          | type     | size    | description   |
| -------- | -------- | ------- | ------------- |
| Encoding | uint16_t | 2 bytes | Encoding type |

Possible values for `Encoding` are:
```
0 = INI encoding
```

### Thumbnail
Image data for thumbnail.
Each thumbnail is defined in its own block.

#### Parameters
|        | type     | size    | description  |
| ------ | -------- | ------- | ------------ |
| Format | uint16_t | 2 bytes | Image format |
| Width  | uint16_t | 2 bytes | Image width  |
| Height | uint16_t | 2 bytes | Image height |

Possible values for `Format` are:
```
0 = PNG format
1 = JPG format
2 = QOI format
```

### Print metadata
Table of key-value pairs of print metadata, such as print time or material consumed, etc.

#### Parameters
|          | type     | size    | description   |
| -------- | -------- | ------- | ------------- |
| Encoding | uint16_t | 2 bytes | Encoding type |


Possible values for `Encoding` are:
```
0 = INI encoding
```

### Slicer metadata
Table of key-value pairs of metadata produced and consumed by the software generating the G-code file.

#### Parameters
|          | type     | size    | description   |
| -------- | -------- | ------- | ------------- |
| Encoding | uint16_t | 2 bytes | Encoding type |

Possible values for `Encoding` are:
```
0 = INI encoding
```

### GCode
G-code data.

#### Parameters
|          | type     | size    | description   |
| -------- | -------- | ------- | ------------- |
| Encoding | uint16_t | 2 bytes | Encoding type |

Possible values for `Encoding` are:
```
0 = No encoding
1 = MeatPack algorithm
2 = MeatPack algorithm modified to keep comment lines
```


