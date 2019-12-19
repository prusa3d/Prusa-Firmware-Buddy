hex2dfu
=======

Simple command line tool to convert file format from Intel HEX to STM32 DFU.

Forked from https://github.com/encedo/hex2dfu

No ED25519 code signing feature support



Compile
=======

hex2dfu is a single C file application and can be easly compile by any ANSI C compiler. No makefile required. Just type:

gcc hex2dfu.c -o hex2dfu.exe


Tested conversions
=====

1. Simple convertion

   hex2dfu.exe -i infile.hex -o outfile.dfu
