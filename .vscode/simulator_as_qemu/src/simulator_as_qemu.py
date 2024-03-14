#!/usr/bin/env python3
import os
import sys
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
        if option[0] == '--scripted':
            # Open TCP port for scripting the simulator
            yield from [
                '-chardev',
                'socket,id=p404-scriptcon,port=50001,host=localhost,server=on',
                '-global', 'p404-scriptcon.no_echo=true'
            ]
        elif option[0] in ('-machine', '-cpu', '-nographic',
                           '-semihosting-config', '-kernel'):
            pass
        else:
            yield from option


def main():
    project_root = Path(__file__).parent.parent.parent.parent
    simulator_path = project_root / 'utils' / 'simulator'
    extra_args = [
        f'-X' + arg for arg in
        simulator_args(sys.argv[1:] +
                       ['-netdev', 'user,id=mini-eth,hostfwd=tcp::3333-:80'])
    ]

    os.execv(sys.executable, [sys.executable, (simulator_path)] + extra_args)


if __name__ == '__main__':
    main()
