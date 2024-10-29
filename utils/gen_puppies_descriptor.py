#!/usr/bin/env python3
import argparse
import hashlib

STORED_TYPE_SIZE = 4
DUMP_OFFSET_SIZE = 4
FINGERPRINT_SIZE = 32
DUMP_SIZE_SIZE = 4

FW_TYPE_CONSTANT = 12321

FW_DESCRIPTOR_SECTION_SIZE = 128
MAX_FW_SIZE_WITH_BL = 120 * 1024
MAX_FW_SIZE_WITHOUT_BL = 128 * 1024


def main():
    parser = argparse.ArgumentParser(
        description='Calculate SHA256 fingerprint of a file.')
    parser.add_argument('input_file')
    parser.add_argument('output_file')
    args = parser.parse_args()

    with open(args.input_file, 'rb') as f:
        data = f.read()
        has_bootloader = len(data) <= MAX_FW_SIZE_WITH_BL
        if has_bootloader:
            descriptor_start = MAX_FW_SIZE_WITH_BL - FW_DESCRIPTOR_SECTION_SIZE
        else:
            descriptor_start = MAX_FW_SIZE_WITHOUT_BL - FW_DESCRIPTOR_SECTION_SIZE

        assert (len(data) >= descriptor_start
                )  # binary needs to have the fw_descriptor section
        data = data[:descriptor_start]

        m = hashlib.sha256()
        m.update(data)
        fingerprint = m.digest()
        assert (len(fingerprint) == FINGERPRINT_SIZE)

        print(f"Puppy fingerprint: {fingerprint.hex()}")

    with open(args.output_file, 'wb') as f:
        # order needs to match exactly the struct in include/puppies/crash_dump_shared.hpp
        f.write(FW_TYPE_CONSTANT.to_bytes(STORED_TYPE_SIZE, 'little'))
        f.write(int(0).to_bytes(DUMP_OFFSET_SIZE, 'little'))
        f.write(fingerprint)
        f.write(int(0).to_bytes(DUMP_SIZE_SIZE, 'little'))


if __name__ == '__main__':
    main()
