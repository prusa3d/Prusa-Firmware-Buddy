#!/usr/bin/env python3

# This script makes it easier to debug crash dumps
# It launches GDB and loads crash dump, then you can use GDB as you normally would to debug crash dump

import argparse
from pathlib import Path
import os, stat, sys
import platform
from shutil import which
from build_dir import find_latest_build_dir

# init directories
script_dir = Path(os.path.dirname(__file__))
project_root_dir = script_dir / ".."
project_root_dir = project_root_dir.resolve()

crash_debug_version = "22acef8c6e248db2f04f65eebf4c2b470f4010c2"

if platform.system() == "Windows":
    crash_debug_path = f'{project_root_dir}/.dependencies/CrashDebug-{crash_debug_version}/bins/win32/CrashDebug'
elif platform.system() == "Linux":
    crash_debug_path = f'{project_root_dir}/.dependencies/CrashDebug-{crash_debug_version}/bins/lin64/CrashDebug'
    os.chmod(crash_debug_path,
             os.stat(crash_debug_path).st_mode | stat.S_IEXEC)
elif platform.system() == "Darwin":
    crash_debug_path = f'{project_root_dir}/.dependencies/CrashDebug-{crash_debug_version}/bins/osx64/CrashDebug'
    os.chmod(crash_debug_path,
             os.stat(crash_debug_path).st_mode | stat.S_IEXEC)

# set arguments
parser = argparse.ArgumentParser(
    prog='CrashDumpDebug',
    description='Launches GDB to debug crash dump',
    epilog='Have fun debugging')
parser.add_argument(
    "--dump",
    type=Path,
    help='crash dump to load; without it GDB only opens the ELF')
parser.add_argument('--elf', type=Path)
parser.add_argument('--gdb', type=Path)
parser.add_argument(
    '--fast',
    action='store_true',
    help=
    'print thread information, stack traces and exit the debugger immediately')
args = parser.parse_args()

# check provided arguments
if (args.elf == None):
    latest = find_latest_build_dir()
    if latest is None:
        print("ELF file not provided and no firmware found under build/*. "
              "Specify --elf explicitly.")
        exit(1)
    args.elf = latest / 'firmware'
    print(f"ELF file not provided, using default: {args.elf}")
if (args.gdb == None):
    args.gdb = f'gdb-multiarch'
    print(f"GDB executable not provided, using default: {args.gdb}")

if not os.path.isfile(args.elf):
    print(f"ELF file not found at: {args.elf}")
    exit(1)

if args.dump is not None and not os.path.isfile(args.dump):
    print(f"Crash dump file not found at: {args.dump}")
    exit(1)
if not os.path.isfile(args.gdb) and which(args.gdb) is None:
    print(f"GDB executable not found at: {args.gdb}")
    exit(1)

# setup command and launch debugger
cmd = f'{args.gdb} {args.elf} -ex "set target-charset ASCII"'

if args.dump is not None:
    cmd += f' -ex "target remote | \\"{crash_debug_path}\\" --elf \\"{args.elf}\\" --dump \\"{args.dump}\\""'

cmd += f' -ex "source {project_root_dir}/utils/freertos-gdb-plugin/freertos-gdb-plugin.py"'
if args.fast:
    if args.dump is None:
        print("--fast needs a crash dump, specify --dump")
        exit(1)

    cmd += f' -ex "set confirm off"'
    cmd += f' -ex "freertos info threads"'
    cmd += f' -ex "freertos thread apply all bt"'
    cmd += f' -ex "quit"'

print("Launching GDB with arguments:")
print(cmd)
os.system(cmd)
