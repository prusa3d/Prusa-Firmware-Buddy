"""Generate bin file for boolader."""
from argparse import ArgumentParser
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
    MINI = 2


class TLVType(Enum):
    """
    Documents known entry types included after the main firmware image in BBF (stored in TLV format)
    """

    RESOURCES_IMAGE = 1  # Littlefs image containing all the resources for the firmware
    RESOURCES_IMAGE_BLOCK_SIZE = 2  # Block size used by the image in RESOURCES_IMAGE (uint32_t, little endian)
    RESOURCES_IMAGE_BLOCK_COUNT = 3  # Block count used by the image in RESOURCES_IMAGE (uint32_t, little endian)
    RESOURCES_IMAGE_HASH = 4  # SHA256 of the RESOURCES_IMAGE entry content

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
    return data


def main():
    parser = ArgumentParser(description=__doc__)
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
        "--tlv", type=str,
        help='TLV data (TYPE:FILE format)', nargs='*', default=[])
    parser.add_argument('-TCI', '--TCI', action='store_true', required=False,
        help='evoked from Travis script')
    # yapf: enable

    args = parser.parse_args()
    version = parse_version(args.version)
    check_byte(args.board, "Board major version")
    printer_type = PrinterType(args.printer_type)  # could raise ValueError
    check_byte(args.printer_version, "Printer type version")
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
        bin_data += printer_type.value.to_bytes(1, 'little')
        bin_data += args.printer_version.to_bytes(1, 'little')
        bin_data += bytes(463)  # aligmnent to 512B (32B for SHA)

    if version.prerelease:
        fmt = "\tversion:  {v.major}.{v.minor}.{v.patch}-{v.prerelease}+{bn}"
    else:
        fmt = "\tversion:  {v.major}.{v.minor}.{v.patch}+{bn}"
    print(fmt.format(v=version, bn=build_number))

    printer_name = printer_type.name
    if args.printer_version > 1:
        printer_name += "%s" % args.printer_version

    print("\tboard: %d" % args.board)
    print("\tprinter: %s" % printer_name)

    with open(args.firmware, "br") as bfile:
        bin_data += bfile.read()

    # TLV Data
    for tlv in args.tlv:
        tlv_entry = tlv.split(":")
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
        sha = sha256(bin_data)
    print("\tsha256sum:", sha.hexdigest())

    sig = bytes(64)  # zeros if not sign
    if not args.no_sign:
        if args.TCI:
            key_str = os.environ.get("sign_key")
            key = SigningKey.from_pem(key_str)
        else:
            key = SigningKey.from_pem(open(args.key).read())
        sig = key.sign(bin_data, hashfunc=sha256)
    print("\tsign:     ", sig.hex())

    # Bootloader Binary File / Firmware
    with open("%s.bbf" % fw_file, "wb") as bbf:
        bbf.write(sig)
        bbf.write(sha.digest())
        bbf.write(bin_data)

    print("Done")
    return 0


if __name__ == "__main__":
    exit(main())
