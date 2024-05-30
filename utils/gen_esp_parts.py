#!/usr/bin/env python3
from dataclasses import dataclass
from typing import List
import argparse
import hashlib
import os
import io
import math


@dataclass
class Spec:
    basename: str
    address: int


def parse_specs(raw_specs: List[str]) -> List[Spec]:
    res = []
    for raw_spec in raw_specs:
        basename, address = raw_spec.split(':')
        address_int = int(address, 16)
        res.append(Spec(basename, address_int))
    res.sort(key=lambda x: x.address)
    return res


def write_part(out: io.IOBase, basedir: str, spec: Spec, chunk_size: int):
    filename = os.path.join(basedir, spec.basename)
    filesize = os.path.getsize(filename)
    bytes_processed = 0
    with open(filename, 'rb') as file:
        while bytes_processed < filesize:
            file_bytes = file.read(chunk_size)
            read_size = len(file_bytes)
            md5 = hashlib.md5(file_bytes).digest()
            md5_str = '{' + ', '.join([f'0x{p:02x}' for p in md5]) + '}'

            out.write('{\n')
            out.write(
                f'    .filename = "/internal/res/esp/{spec.basename}",\n')
            out.write(
                f'    .address = 0x{spec.address + bytes_processed:x},\n')
            out.write(f'    .size = 0x{read_size:x},\n')
            out.write(f'    .offset = 0x{bytes_processed:x},\n')
            out.write(f'    .md5 = {md5_str},\n')
            out.write('},\n')
            bytes_processed += read_size
    return bytes_processed


def write_parts(out: io.IOBase, basedir: str, name: str, specs: List[Spec],
                chunk_size: int, buffer_size: int):
    out.write(f'constexpr Part {name}[] = {{\n')
    last_top = 0
    last_filename = ""
    for spec in specs:
        assert last_top <= spec.address, f"{last_filename} overlaps with {spec.basename} on 0x{spec.address:x}"
        bytes_processed = write_part(out, basedir, spec, chunk_size)
        bytes_processed_with_padding = math.ceil(
            bytes_processed / buffer_size) * buffer_size
        last_top = spec.address + bytes_processed_with_padding
        last_filename = spec.basename
    out.write('};\n')


def main():
    parser = argparse.ArgumentParser(description='Generate esp parts')
    parser.add_argument('--output')
    parser.add_argument('--basedir')
    parser.add_argument('--write-buffer-size', type=int, default=512)
    parser.add_argument('--max-chunk-size', type=int, default=0x10000)
    parser.add_argument('--flash', action='append')
    parser.add_argument('--memory', action='append')
    args = parser.parse_args()
    assert args.max_chunk_size % args.write_buffer_size == 0, f"max-chunk-size{args.max_chunk_size} must be divisible by write-buffer-size{args.write_buffer_size} to prevent overlaping rewrites with padding"

    with open(args.output, 'w') as out:
        out.write('namespace esp::flash {\n')
        out.write(
            f'constexpr size_t buffer_size = {args.write_buffer_size};\n')
        basedir = args.basedir
        write_parts(out, basedir, 'flash_parts', parse_specs(args.flash or []),
                    args.max_chunk_size, args.write_buffer_size)
        write_parts(out, basedir, 'memory_parts', parse_specs(args.memory
                                                              or []),
                    args.max_chunk_size, args.write_buffer_size)
        out.write('constexpr uintptr_t memory_entry = 0x4010d004;\n')
        out.write('}\n')


if __name__ == '__main__':
    main()
