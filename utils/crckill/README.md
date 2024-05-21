# README

A simple tool that can take a bgcode with CRCs and:
a) List all the blocks there.
b) Mangle the CRCs of selected blocks.

Testing purposes, to create a „corrupt“ gcode file (the actual content is left
intact and we damage the CRC, but that's the same from the checking
perspective).

How to use:

First, run once without killing any of the CRCs. This'll output list of the
blocks, identified by numbers (and create an identical output.bgcode).

```
cargo run -- input.bgcode output.bgcode
```

Now, pick any number of blocks to kill and list them:

```
cargo run -- input.bgcode output.bgcode -k 3 -k 13
```
