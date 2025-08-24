#!/usr/bin/env python3

from argparse import ArgumentParser
from io import BytesIO
import os


def write_bytes_to_file(filename, data):
    if not data:
        return

    if os.path.exists(filename):
        print(f'Refusing to overwrite {filename}')
        return

    with open(filename, 'wb') as f:
        f.write(data)
    print(f'Extracted {filename}')


def main():
    parser = ArgumentParser(description='Unpack tool for BBF files')
    parser.add_argument('--input-file',
                        type=str,
                        required=True,
                        help='path to the input .bbf file')
    args = parser.parse_args()

    def parse_int(b):
        return int.from_bytes(b, 'little')

    with open(args.input_file, 'rb') as bbf:
        signature = bbf.read(64)
        digest = bbf.read(32)
        firmware_length = parse_int(bbf.read(4))
        firmware_version_major = parse_int(bbf.read(1))
        firmware_version_minor = parse_int(bbf.read(1))
        firmware_version_patch = parse_int(bbf.read(1))
        firmware_version_build_number = parse_int(bbf.read(2))
        firmware_version_prerelease = bbf.read(5)
        buddy_board_revision = parse_int(bbf.read(1))
        printer_type = parse_int(bbf.read(1))
        printer_version_or_bbf_version = parse_int(bbf.read(1))
        tlv = {}
        if (printer_version_or_bbf_version == 1):
            bbf_version = 1
            printer_version = printer_version_or_bbf_version
            printer_subversion = parse_int(bbf.read(1))
            _ = bbf.read(462)
            firmware_code = bbf.read(firmware_length)
        else:
            bbf_version = printer_version_or_bbf_version
            printer_subversion = parse_int(bbf.read(1))
            printer_version = parse_int(bbf.read(1))
            _ = bbf.read(461)
            firmware_code = bbf.read(firmware_length)
            tlv_data = BytesIO(bbf.read())
            while True:
                tlv_type = parse_int(tlv_data.read(1))
                tlv_length = parse_int(tlv_data.read(4))
                tlv_payload = tlv_data.read(tlv_length)
                if not tlv_type:
                    break
                else:
                    tlv[tlv_type] = tlv_payload

    print(f'bbf version: {bbf_version}')
    print(
        f'firmware version: {firmware_version_major}.{firmware_version_minor}.{firmware_version_patch}+{firmware_version_build_number}'
    )
    print(f'buddy board revision: {buddy_board_revision}')
    print(f'printer type: {printer_type}')
    print(f'printer version: {printer_version}')
    print(f'printer subversion: {printer_subversion}')

    write_bytes_to_file('firmware.bin', firmware_code)
    write_bytes_to_file('resources-image.lfs', tlv.get(1, bytes()))

    return 0


if __name__ == '__main__':
    exit(main())
