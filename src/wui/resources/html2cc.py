#!/bin/env python3

# from zlib import compress
from os import listdir
from os.path import basename, splitext

from requests import post


def convert(filename):
    base, ext = splitext(basename(filename))

    with open(filename, "br") as src:
        cdata = src.read()
        if ext == ".css":
            cdata = post('https://cssminifier.com/raw',
                         data=dict(input=cdata)).text.encode('utf-8')
        elif ext == ".js":
            cdata = post('https://javascript-minifier.com/raw',
                         data=dict(input=cdata)).text.encode('utf-8')
        elif ext == ".html":
            cdata = post('https://html-minifier.com/raw',
                         data=dict(input=cdata)).text.encode('utf-8')

        # cdata = compress(src.read())
        with open("cc_wsapi/%s_%s.c" % (base, ext[1:]), "w+") as csource:
            print("convert %s -> %s" % (src.name, csource.name))
            csource.write("#include <inttypes.h>\n\n")
            csource.write("const uint8_t %s_%s[] =\n" % (base, ext[1:]))
            csource.write("{")
            col = 0
            for b in cdata:
                if col == 0:
                    csource.write("\n\t")
                elif col % 12 == 0:
                    csource.write(",\n\t")
                else:
                    csource.write(", ")
                csource.write("0x%02x" % b)
                col += 1
            csource.write("\n};")
            csource.flush()


for filename in listdir("src"):
    if filename[0] != '.' and filename[-1] != '~':
        convert("src/" + filename)
