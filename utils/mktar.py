#!/usr/bin/env python3
"""Build the tarball.

Usage:
    mktar.py <output.tar> [<target> <source> ...]
"""
import sys
import tarfile
from pathlib import Path


def validate_name(name):
    # Keep the name rooted so firmware doesn't have to prepend '/'
    if not name.startswith('/'):
        raise SystemExit(f'mktar.py: {name} is not rooted')
    # Keep the name short so firmware doesn't have to support ustar prefix
    encoded = name.encode('utf-8')
    if len(encoded) > 100:
        raise SystemExit(f'mktar.py: {name} too long')


def main():
    if len(sys.argv) < 2 or len(sys.argv[2:]) % 2:
        raise SystemExit(
            'Usage: mktar.py <output.tar> [<target> <source> ...]')
    output, *entries = sys.argv[1:]
    members = sorted((target, source)
                     for target, source in zip(entries[0::2], entries[1::2]))

    # Also add directory entries so firmware can create them before files
    dirs = set()
    for name, _ in members:
        dirname = '/'
        for part in name.split('/')[1:-1]:
            dirname += part + '/'
            dirs.add(dirname)

    with tarfile.open(output, 'w', format=tarfile.USTAR_FORMAT) as tf:
        for name in sorted(dirs):
            validate_name(name)
            info = tarfile.TarInfo(name)
            info.type = tarfile.DIRTYPE
            tf.addfile(info)
        for name, source in members:
            validate_name(name)
            info = tarfile.TarInfo(name)
            info.size = Path(source).stat().st_size
            with open(source, 'rb') as f:
                tf.addfile(info, f)


if __name__ == '__main__':
    main()
