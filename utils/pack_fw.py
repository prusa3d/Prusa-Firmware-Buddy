"""Generate bin file for boolader."""
from argparse import ArgumentParser
from os.path import splitext
from hashlib import sha256
from enum import Enum

import re
import os

from ecdsa import SigningKey

re_semver = re.compile(
    r"""^(?P<major>[0-9]{1,3})\.
        (?P<minor>[0-9]{1,3})\.
        (?P<patch>[0-9]{1,3})
        (-(?P<prerelease>\w{1,5}))?
        (\+(?P<build>[0-9]{1,5}))$
    """, re.X)


class PrinterType(Enum):
    MINI = 2


class UndefinedSha256():
    def digest(self):
        return b'\xff' * 32

    def hexdigest(self):
        return self.digest().hex()


def check_version(string):
    match = re_semver.search(string)
    if not match:
        raise ValueError("Not valid version")
    vdict = match.groupdict()
    for it in ("major", "minor", "patch"):
        vdict[it] = int(vdict[it])
        if vdict[it] > 256:
            raise ValueError("%s number is too high!" % it.title())
    vdict["build"] = int(vdict["build"])
    if vdict["build"] > 65365:
        raise ValueError("Build number is too high!")
    vdict['prerelease'] = vdict.get('prerelease') or ''
    return vdict


def check_byte(number, name):
    """Check that number could be stored to one byte."""
    if number > 255:
        raise ValueError("%s must be 0 - 255!", name)


def write_version(ver):
    """Return bytes[10] from version dictionary."""
    data = bytes()
    for it in ("major", "minor", "patch"):
        data += ver[it].to_bytes(1, 'little')
    data += ver["build"].to_bytes(2, 'little')
    data += ver["prerelease"].ljust(5, '\0').encode()
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
    parser.add_argument('-TCI', '--TCI', action='store_true', required=False,
        help='evoked from Travis script')
    # yapf: enable

    args = parser.parse_args()
    version = check_version(args.version)
    check_byte(args.board, "Board major version")
    printer_type = PrinterType(args.printer_type)  # could raise ValueError
    check_byte(args.printer_version, "Printer type version")
    fw_file = splitext(args.firmware)[0]
    bin_data = bytes()

    print("Processing ", args.firmware)
    with open(args.firmware, "br") as bfile:
        bin_data += bfile.seek(0, 2).to_bytes(4, 'little')  # firmware length
        bin_data += write_version(version)  # 10 bytes
        bin_data += args.board.to_bytes(1, 'little')
        bin_data += printer_type.value.to_bytes(1, 'little')
        bin_data += args.printer_version.to_bytes(1, 'little')
        bin_data += bytes(463)  # aligmnent to 512B (32B for SHA)

    if version["prerelease"]:
        print("\tversion:  {major}.{minor}.{patch}-{prerelease}+{build}"
              "".format(**version))
    else:
        print("\tversion:  {major}.{minor}.{patch}+{build}"
              "".format(**version))

    printer_name = printer_type.name
    if args.printer_version > 1:
        printer_name += "%s" % args.printer_version

    print("\tboard: %d" % args.board)
    print("\tprinter: %s" % printer_name)

    with open(args.firmware, "br") as bfile:
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
