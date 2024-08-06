"""Generate bin file for boolader."""
from argparse import ArgumentParser, Action
from os.path import splitext
from hashlib import sha256
from enum import Enum
from collections import namedtuple

import re
import os

from ecdsa import SigningKey

re_semver = re.compile(
    r"""
    ^(?P<major>0|[1-9]\d*)\.
    (?P<minor>0|[1-9]\d*)\.
    (?P<patch>0|[1-9]\d*)
    (?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)
        (?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*
        ))?
    (?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$
    """, re.X)

SemVer = namedtuple('SemVer',
                    ['major', 'minor', 'patch', 'prerelease', 'buildmetadata'])


class PrinterType(Enum):
    MK4 = 1
    MINI = 2
    XL = 3
    iX = 4
    MK3point5 = 5
    CUBE = 6


class TLVType(Enum):
    """
    Documents known entry types included after the main firmware image in BBF (stored in TLV format)
    """

    RESOURCES_IMAGE = 1  # Littlefs image containing all the resources for the firmware
    RESOURCES_IMAGE_BLOCK_SIZE = 2  # Block size used by the image in RESOURCES_IMAGE (uint32_t, little endian)
    RESOURCES_IMAGE_BLOCK_COUNT = 3  # Block count used by the image in RESOURCES_IMAGE (uint32_t, little endian)
    RESOURCES_IMAGE_HASH = 4  # SHA256 of the RESOURCES_IMAGE entry content

    RESOURCES_BOOTLOADER_IMAGE = 5  # Littlefs image containing bootloader.bin in the root
    RESOURCES_BOOTLOADER_IMAGE_BLOCK_SIZE = 6  # Block size used by the image in RESOURCES_BOOTLOADER_IMAGE (uint32_t, little endian)
    RESOURCES_BOOTLOADER_IMAGE_BLOCK_COUNT = 7  # Block count used by the image in RESOURCES_BOOTLOADER_IMAGE (uint32_t, little endian)
    RESOURCES_BOOTLOADER_IMAGE_HASH = 8  # SHA256 of the RESOURCES_BOOTLOADER_IMAGE content

    @staticmethod
    def from_user_spec(spec: str) -> int:
        try:
            return TLVType[spec.upper()].value
        except KeyError:
            return int(spec)


class UndefinedSha256():

    def digest(self):
        return b'\xff' * 32

    def hexdigest(self):
        return self.digest().hex()


def parse_version(string) -> SemVer:
    match = re_semver.search(string)
    if not match:
        raise ValueError('invalid version string')
    vdict = match.groupdict()
    for it in ("major", "minor", "patch"):
        vdict[it] = int(vdict[it])
        if vdict[it] > 256:
            raise ValueError("%s number is too high!" % it.title())
    if vdict.get('buildmetadata'):
        vdict['buildmetadata'] = vdict['buildmetadata'].split('.')
    else:
        vdict['buildmetadata'] = []
    vdict['prerelease'] = vdict.get('prerelease') or ''
    return SemVer(**vdict)


def check_byte(number, name):
    """Check that number could be stored to one byte."""
    if number > 255:
        raise ValueError("%s must be 0 - 255!", name)


def get_build_number(version: SemVer) -> int:
    for build_comp in version.buildmetadata:
        try:
            return int(build_comp)
        except ValueError:
            pass
    else:
        return None


def write_version(ver, *, build_number: int):
    """Return bytes[10] from version dictionary."""
    data = bytes()
    for it in ("major", "minor", "patch"):
        data += getattr(ver, it).to_bytes(1, 'little')
    data += build_number.to_bytes(2, 'little')
    data += ver.prerelease.ljust(5, '\0').encode()
    if len(data) != 10:
        raise ValueError('version must be 10 bytes long')
    return data


class ExtendAction(Action):

    def __call__(self, parser, namespace, values, option_string=None):
        items = getattr(namespace, self.dest) or []
        items.extend(values)
        setattr(namespace, self.dest, items)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.register('action', 'extend', ExtendAction)
    # yapf: disable
    parser.add_argument(
        "firmware", type=str,
        help='Raw firware file', metavar="<FILE>")
    parser.add_argument(
        "-k", "--key", default="private.key", type=str, required=False,
        help='PEM EC private key file', metavar="<FILE>")
    parser.add_argument(
        "-v", "--version", type=str, required=True,
        help='Firmware version in format MAJOR.MINOR.PATCH[-RELEASE]+BUILD, '
             'Which is uint8.uint8.uint8[-char[5]]+uint16')
    parser.add_argument(
        "--build-number", type=int, required=False,
        help='In case the version specified with --version does not contain'
        ' a build number, it can be specified using this option.')
    parser.add_argument(
        "-n", "--no-sign", action="store_true",
        help="Do not sign firmware - for developers only!")
    parser.add_argument(
        "-c", "--no-checksum", action="store_true",
        help="Do not create checksum - for developers only!")
    parser.add_argument(
        "-b", "--board", type=int, default=0,
        help='Board major version')
    parser.add_argument(
        "-t", "--printer-type", type=int, required=True,
        help="Printer type from firmware enum.")
    parser.add_argument(
        "-V", "--printer-version", type=int, required=True,
        help="Printer version of printer type.")
    parser.add_argument(
        "--printer-subversion", type=int, required=True,
        help="Printer subversion of printer type.")
    parser.add_argument(
        "--tlv", type=str,
        help='TLV data (TYPE:FILE format)', action='extend', nargs='*', default=[])
    parser.add_argument(
        "--bbf-version", type=int, required=False, default=2,
        help="Version of the BBF. 1: original first version, 2: version with TLV extension")
    # yapf: enable

    args = parser.parse_args()
    version = parse_version(args.version)
    check_byte(args.board, "Board major version")
    printer_type = PrinterType(args.printer_type)  # could raise ValueError
    check_byte(args.printer_version, "Printer type version")
    check_byte(args.printer_subversion, "Printer type subversion")
    fw_file = splitext(args.firmware)[0]
    if args.build_number is not None:
        build_number = args.build_number
    else:
        build_number = get_build_number(version)
        if build_number is None:
            raise ValueError('unknown build number')
    bin_data = bytes()

    print("Processing ", args.firmware)
    with open(args.firmware, "br") as bfile:
        bin_data += bfile.seek(0, 2).to_bytes(4, 'little')  # firmware length
        bin_data += write_version(version,
                                  build_number=build_number)  # 10 bytes
        bin_data += args.board.to_bytes(1, 'little')
        was_cube = False
        if printer_type == PrinterType.CUBE:
            printer_type = PrinterType.MK4
            was_cube = True
        bin_data += printer_type.value.to_bytes(1, 'little')
        if was_cube:
            printer_type = PrinterType.CUBE
        if args.bbf_version == 1:
            bin_data += args.printer_version.to_bytes(1, 'little')
            bin_data += args.printer_subversion.to_bytes(1, 'little')
            bin_data += bytes(1)
        elif args.bbf_version == 2:
            bin_data += args.bbf_version.to_bytes(1, 'little')
            if printer_type == PrinterType.CUBE:
                bin_data += (0).to_bytes(1, 'little')
                bin_data += (4).to_bytes(1, 'little')
            else:
                bin_data += args.printer_subversion.to_bytes(1, 'little')
                bin_data += args.printer_version.to_bytes(1, 'little')
        bin_data += bytes(461)  # aligmnent to 512B (32B for SHA)

    if version.prerelease:
        fmt = "\tversion:  {v.major}.{v.minor}.{v.patch}-{v.prerelease}+{bn}"
    else:
        fmt = "\tversion:  {v.major}.{v.minor}.{v.patch}+{bn}"
    print(fmt.format(v=version, bn=build_number))

    printer_name = printer_type.name
    if args.printer_version > 1:
        printer_name += "%s" % args.printer_version
    if args.printer_subversion > 0:
        printer_name += ".%s" % args.printer_subversion

    print("\tboard: %d" % args.board)
    print("\tprinter: %s" % printer_name)

    with open(args.firmware, "br") as bfile:
        bin_data += bfile.read()

    bin_data_fw_only = bin_data

    # TLV Data
    for tlv in args.tlv:
        assert args.bbf_version >= 2, "TLV extension is supported starting with BBF version 2"
        tlv_entry = tlv.split(":", maxsplit=1)
        assert len(tlv_entry) == 2, "For -tlv use format TYPE:FILE"
        type = TLVType.from_user_spec(tlv_entry[0])
        path = tlv_entry[1]
        length = os.stat(path).st_size

        bin_data += type.to_bytes(1, 'little')
        bin_data += length.to_bytes(4, 'little')
        with open(path, "br") as bfile:
            bin_data += bfile.read()

    sha = UndefinedSha256()
    if not args.no_checksum:
        sha = sha256(bin_data_fw_only)
    print("\tsha256sum:", sha.hexdigest())

    sig = bytes(64)  # zeros if not sign
    if not args.no_sign:
        key = SigningKey.from_pem(open(args.key).read())
        sig = key.sign(bin_data_fw_only, hashfunc=sha256)
    print("\tsign:     ", sig.hex())

    # Bootloader Binary File / Firmware
    with open(f"{fw_file}.bbf", "wb") as bbf:
        bbf.write(sig)
        bbf.write(sha.digest())
        bbf.write(bin_data)

    print("Done")
    return 0


if __name__ == "__main__":
    exit(main())
