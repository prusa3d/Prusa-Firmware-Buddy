#!/usr/bin/env python3
import argparse
import hashlib
import os

# Max number of bytes that should be flashed at once
MAX_SEND_SIZE = 0x10000


def write_part(out, basedir, spec):
    basename, address = spec.split(':')
    address = int(address, 16)
    filename = os.path.join(basedir, basename)
    filesize = os.path.getsize(filename)
    bytes_processed = 0
    with open(filename, 'rb') as file:
        while bytes_processed < filesize:
            file_bytes = file.read(MAX_SEND_SIZE)
            chunk_size = len(file_bytes)
            md5 = hashlib.md5(file_bytes).digest()
            md5 = '{' + ', '.join([f'0x{p:02x}' for p in md5]) + '}'

            out.write('{\n')
            out.write(f'    .filename = "/internal/res/esp/{basename}",\n')
            out.write(f'    .address = 0x{address + bytes_processed:x},\n')
            out.write(f'    .size = 0x{chunk_size:x},\n')
            out.write(f'    .offset = 0x{bytes_processed:x},\n')
            out.write(f'    .md5 = {md5},\n')
            out.write('},\n')
            bytes_processed += chunk_size


def write_parts(out, basedir, name, specs):
    out.write(f'static constexpr Part {name}[] = {{\n')
    for spec in specs:
        write_part(out, basedir, spec)
    out.write('};\n')


def main():
    parser = argparse.ArgumentParser(description='Generate esp parts')
    parser.add_argument('--output')
    parser.add_argument('--basedir')
    parser.add_argument('--flash', action='append')
    parser.add_argument('--memory', action='append')
    args = parser.parse_args()

    with open(args.output, 'w') as out:
        basedir = args.basedir
        write_parts(out, basedir, 'flash_parts', args.flash or [])
        write_parts(out, basedir, 'memory_parts', args.memory or [])
        out.write('static constexpr uintptr_t memory_entry = 0x4010d004;\n')


if __name__ == '__main__':
    main()
