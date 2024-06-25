#!/usr/bin/env python3
import sys
import argparse
import struct
import zlib
from typing import List
from pathlib import Path


class DfuImage:

    def __init__(self, address, data):
        self.address = address
        self.data = data


class DfuTarget:

    def __init__(self, images: List[DfuImage], name='Generic Target'):
        self.images = images
        self.name = name


def compute_crc(data):
    return 0xFFFFFFFF & -zlib.crc32(data) - 1


def dfu_file_create_prefix(*, dfu_file_size, targets_count) -> bytes:
    return struct.pack('<5sBIB', b'DfuSe', 0x01, dfu_file_size, targets_count)


def dfu_file_create_target(target: DfuTarget) -> bytes:
    images_data = bytes()
    for image in target.images:
        images_data += struct.pack('<II', image.address, len(image.data))
        images_data += image.data
    target_prefix = struct.pack('<6sBI255s2I', b'Target', 0, 1,
                                target.name.encode('utf-8'), len(images_data),
                                len(target.images))
    return target_prefix + images_data


def dfu_file_append_suffix(data,
                           *,
                           bcd_device=0x0000,
                           id_product=0x0000,
                           id_vendor=0x0000) -> bytes:
    data += struct.pack('<HHHH3sB', bcd_device, id_product, id_vendor, 0x011A,
                        b'UFD', 16)
    data += struct.pack('<I', compute_crc(data))
    return data


def dfu_file_create(path, id_product, id_vendor, targets: List[DfuTarget]):
    targets_data = bytes()
    for target in targets:
        targets_data += dfu_file_create_target(target)
    prefix_data = dfu_file_create_prefix(dfu_file_size=len(targets_data) + 11,
                                         targets_count=len(targets))
    data = prefix_data + targets_data
    data = dfu_file_append_suffix(data,
                                  id_product=id_product,
                                  id_vendor=id_vendor)
    with open(path, 'wb') as f:
        f.write(data)


def device_id_spec(arg):
    return tuple(map(lambda s: int(s, 0) & 0xffff, arg.split(':', 1)))


def input_file_spec(arg):
    address, filepath = arg.split(':', 1)
    return int(address, 0), Path(filepath)


def create_cmd(args):
    images = []
    # load input files
    for infile in args.input_files:
        with open(infile[1], 'rb') as f:
            data = f.read()
            if infile[1].suffix == '.bbf':
                # skip signature in .bbf files
                data = data[64:]
                # next is the header (which we want to keep & flash)
                # it contains 32B with the firmware hash
                # followed by 4B with firmware size
                firmware_size = int.from_bytes(data[32:32 + 4], 'little')
                # strip resources and other metadata following the firmware itself
                # firmware starts after the first 512B (the header)
                data = data[:512 + firmware_size]
            images.append(DfuImage(infile[0], data))
    # create dfu files
    dfu_file_create(path=args.dfu_file,
                    id_product=args.device[0],
                    id_vendor=args.device[1],
                    targets=[DfuTarget(images=images)])


def main(argv):
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()
    # yapf: disable
    parser_create = subparsers.add_parser('create', help='create a dfu file')
    parser_create.add_argument(
        '--device', type=device_id_spec, default='0x0000:0x0483')
    parser_create.add_argument(
        'input_files', nargs='+', type=input_file_spec,
        help='Input binary files and their addresses, eg. "0x0:./firmware.bin"')
    parser_create.add_argument(
        'dfu_file', type=Path,
        help='Path where the output file should be saved')
    parser_create.set_defaults(func=create_cmd)
    # yapf: enable
    args = parser.parse_args(argv[1:])
    try:
        args.func(args)
    except AttributeError:
        parser.parse_args(['--help'])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
