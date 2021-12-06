#!/usr/bin/env python3

import sys

import os
from pathlib import Path
import gzip


def generate_header():
    print('#include <lwip/apps/fs.h>')
    print('#include <lwip/def.h>')

    print('#define file_NULL (struct fsdata_file *)NULL\n')

    print('#ifndef FS_FILE_FLAGS_HEADER_INCLUDED')
    print('#define FS_FILE_FLAGS_HEADER_INCLUDED 1')
    print('#endif')
    print('#ifndef FS_FILE_FLAGS_HEADER_PERSISTENT')
    print('#define FS_FILE_FLAGS_HEADER_PERSISTENT 0')
    print('#endif')
    print('#ifndef FSDATA_FILE_ALIGNMENT')
    print('#define FSDATA_FILE_ALIGNMENT 0')
    print('#endif')
    print('#ifndef FSDATA_ALIGN_PRE')
    print('#define FSDATA_ALIGN_PRE')
    print('#endif')
    print('#ifndef FSDATA_ALIGN_POST')
    print('#define FSDATA_ALIGN_POST')
    print('#endif')
    print('#if FSDATA_FILE_ALIGNMENT == 2')
    print('#include "fsdata_alignment.h"')
    print('#endif\n')


def to_hex(data):
    return ','.join(map(lambda b: hex(b), data)) + ','


def get_hex(file_path):
    with open(file_path, 'rb') as file:
        data = file.read()
    return to_hex(gzip.compress(data))


def process_file(fname, previous):
    # file name
    file_name = Path(fname).name

    # define file array
    sanitized_name = file_name.replace('.', '_')
    print('static const unsigned char FSDATA_ALIGN_PRE data__' +
          sanitized_name + '[] FSDATA_ALIGN_POST = {')

    print(to_hex(('/' + file_name + '\0').encode('ascii')))

    # hex file data
    print(get_hex(fname))

    print('};\n')

    name_len = len(file_name) + 2
    print('const struct fsdata_file file__' + sanitized_name + '[] = { {')
    print(previous + ',')
    print('data__' + sanitized_name + ',')
    print('data__' + sanitized_name + ' + ' + str(name_len) + ',')
    print('sizeof(data__' + sanitized_name + ') - ' + str(name_len) + ',')
    print('0,')
    print('} };\n')

    return 'file__' + sanitized_name


def finish(last, cnt):
    # add define to file num & root
    print('#define FS_NUMFILES ' + str(cnt))
    print('#define FS_ROOT ' + last)


def main():
    """
    Accept list of files on command line and produce a c file with hex-arrays
    for the consumption by the wui httpd server.
    """
    generate_header()

    previous = 'file_NULL'
    for fname in sys.argv[1:]:
        previous = process_file(fname, previous)

    finish(previous, len(sys.argv) - 1)


if __name__ == '__main__':
    main()
