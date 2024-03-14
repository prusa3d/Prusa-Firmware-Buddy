#!/usr/bin/python

import struct

# Unfortunately, python considers the first part of the page as "control
# characters". We need the "dingbats" alternative, so this part is manually
# copied from https://en.wikipedia.org/wiki/Code_page_437.
intro = (
    0x0000,
    0x263A,
    0x263B,
    0x2665,
    0x2666,
    0x2663,
    0x2660,
    0x2022,
    0x25D8,
    0x25CB,
    0x25D9,
    0x2642,
    0x2640,
    0x266A,
    0x266B,
    0x263C,
    0x25BA,
    0x25C4,
    0x2195,
    0x203C,
    0x00B6,
    0x00A7,
    0x25AC,
    0x21A8,
    0x2191,
    0x2193,
    0x2192,
    0x2190,
    0x221F,
    0x2194,
    0x25B2,
    0x25BC,
)

for c in intro:
    print(f'0x{c:04X},')

# Identical to ascii/utf8
for c in range(32, 127):
    print(f'0x{c:04X},')

# Again, a control character vs dingbat

print("0x2302,")

# Pulling the rest out of python's conversion table
for c in range(128, 256):
    uni_str = bytes([c]).decode('437').encode('utf16')
    # Starts with bom, there's exactly one 16-bit unit after it.
    assert (len(uni_str) == 4)
    assert (uni_str[:2] == b'\xff\xfe')

    codepoint = struct.unpack('<H', uni_str[2:])[0]
    print(f'0x{codepoint:04X},')
