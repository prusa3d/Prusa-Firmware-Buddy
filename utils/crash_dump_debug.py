# This script makes it easier to debug crash dumps
# It launches GDB and loads crash dump, then you can use GDB as you normally would to debug crash dump

import argparse
from pathlib import Path
import os, stat
import platform

# init directories
script_dir = Path(os.path.dirname(__file__))
project_root_dir = script_dir / ".."
project_root_dir = project_root_dir.resolve()

if platform.system() == "Windows":
    crash_debug_path = f'{project_root_dir}/.dependencies/CrashDebug-ae191d/bins/win32/CrashDebug'
elif platform.system() == "Linux":
    crash_debug_path = f'{project_root_dir}/.dependencies/CrashDebug-ae191d/bins/lin64/CrashDebug'
    os.chmod(crash_debug_path,
             os.stat(crash_debug_path).st_mode | stat.S_IEXEC)
elif platform.system() == "Darwin":
    crash_debug_path = f'{project_root_dir}/.dependencies/CrashDebug-ae191d/bins/osx64/CrashDebug'
    os.chmod(crash_debug_path,
             os.stat(crash_debug_path).st_mode | stat.S_IEXEC)

# set arguments
parser = argparse.ArgumentParser(
    prog='CrashDumpDebug',
    description='Launches GDB to debug crash dump',
    epilog='Have fun debugging')
parser.add_argument("--dump", type=Path, required=True)
parser.add_argument('--elf', type=Path)
parser.add_argument('--gdb', type=Path)
args = parser.parse_args()

# check provided arguments
if (args.elf == None):
    args.elf = project_root_dir / Path('build-vscode-buddy/firmware')
    print(f"ELF file not provided, using default: {args.elf}")
if (args.gdb == None):
    args.gdb = f'{project_root_dir}/.dependencies/gcc-arm-none-eabi-10.3.1/bin/arm-none-eabi-gdb'
    print(f"GDB executable not provided, using default: {args.gdb}")

if not os.path.isfile(args.elf):
    print(f"ELF file not fount at: {args.elf}")
    exit(1)

if not os.path.isfile(args.dump):
    print(f"Crash dump file not fount at: {args.dump}")
    exit(1)

if not os.path.isfile(args.gdb):
    print(f"GDB executable not fount at: {args.gdb}")
    exit(1)

# setup command and launch debugger
cmd = f'{args.gdb} {args.elf} -ex "set target-charset ASCII" -ex "target remote | {crash_debug_path} --elf {args.elf} --dump {args.dump}"'
print("Launching GDB with arguments:")
print(cmd)
os.system(cmd)
