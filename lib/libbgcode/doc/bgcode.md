# bgcode

bgcode is a command line application which allows to convert gcode files from ascii to binary format and viceversa.

## Usage

### Binary to ascii

To convert a gcode file from binary to ascii format, run:
```
bgcode my_gcode.bgcode
```
A new file my_gcode.gcode will be produced.

### Ascii to binary

Conversion of a gcode file from ascii to binary format, requires the specification of the parameters to use during the binarization process.
These parameters are:

#### checksum

The algorithm to use for checksum. 
Possible values:
* 0 - No checksum applied
* 1 - CRC32 algorithm

Default value: `1`
 
#### file_metadata_compression

The compression algorithm to apply to file metadata block. 
Possible values:
* 0 - No compression
* 1 - Deflate algorithm
* 2 - Heatshrink algorithm with window size 11 and lookahead size 4 
* 3 - Heatshrink algorithm with window size 12 and lookahead size 4

Default value: `0`

#### print_metadata_compression

The compression algorithm to apply to print metadata block. 
Possible values:
* 0 - No compression
* 1 - Deflate algorithm
* 2 - Heatshrink algorithm with window size 11 and lookahead size 4 
* 3 - Heatshrink algorithm with window size 12 and lookahead size 4

Default value: `0`

#### printer_metadata_compression

The compression algorithm to apply to printer metadata block. 
Possible values:
* 0 - No compression
* 1 - Deflate algorithm
* 2 - Heatshrink algorithm with window size 11 and lookahead size 4 
* 3 - Heatshrink algorithm with window size 12 and lookahead size 4

Default value: `0`

#### slicer_metadata_compression

The compression algorithm to apply to slicer metadata block. 
Possible values:
* 0 - No compression
* 1 - Deflate algorithm
* 2 - Heatshrink algorithm with window size 11 and lookahead size 4 
* 3 - Heatshrink algorithm with window size 12 and lookahead size 4

Default value: `0`

#### gcode_compression

The compression algorithm to apply to gcode blocks. 
Possible values:
* 0 - No compression
* 1 - Deflate algorithm
* 2 - Heatshrink algorithm with window size 11 and lookahead size 4 
* 3 - Heatshrink algorithm with window size 12 and lookahead size 4

Default value: `0`

#### gcode_encoding

The encoding algorithm to apply to gcode blocks. 
Possible values:
* 0 - No encoding
* 1 - MeatPack algorithm
* 2 - MeatPack algorithm modified to keep comment lines

Default value: `0`

#### metadata_encoding

The encoding algorithm to apply to metadata. 
Possible values:
* 0 - INI algorithm

Default value: `0`

### Example

For example to convert a gcode file from ascii to binary format, with the following settins:

```
gcode_encoding              = MeatPack algorithm modified to keep comment lines
gcode_compression           = Heatshrink algorithm with window size 12 and lookahead size 4
slicer_metadata_compression = Deflate algorithm
```

run:
```
bgcode my_gcode.gcode --slicer_metadata_compression=1 --gcode_compression=3 --gcode_encoding=2
```

To convert the same file, using default settings, run:
```
bgcode my_gcode.gcode
```

In both cases, a new file my_gcode.bgcode will be produced.