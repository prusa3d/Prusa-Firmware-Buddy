#!/usr/bin/env python3
import os
import sys
import subprocess
from pathlib import Path


def options(args):
    while args:
        arg = args.pop(0)
        assert arg.startswith('-')
        option = [arg]
        while args and not args[0].startswith('-'):
            option.append(args.pop(0))
        yield option


def simulator_args(qemu_args):
    for option in options(qemu_args):
        if option[0] in ('-machine', '-cpu', '-nographic',
                         '-semihosting-config', '-kernel'):
            pass
        else:
            yield from option


project_root = Path(__file__).parent.parent
simulator_path = project_root / 'utils' / 'simulator'
extra_args = [f'-X' + arg for arg in simulator_args(sys.argv[1:])]

os.execv(sys.executable, [sys.executable, (simulator_path)] + extra_args)
