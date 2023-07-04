from io import UnsupportedOperation
import shutil
import shlex
import sys
import re
import asyncio
import functools
import logging
from pathlib import Path
from contextlib import AsyncExitStack
from typing import Optional
import platform

import click

logging.basicConfig(level=logging.DEBUG)

# add the 'utils' dir to path so we can see the other packages
project_root = Path(__file__).absolute().parent.parent.parent
utils_dir = project_root / 'utils'
sys.path.insert(0, str(utils_dir))

from pathlib import Path
from simulator.simulator import Simulator
from simulator.printer import MachineType
from bootstrap import get_dependency_directory


class BuildDirectory:
    def __init__(self, directory: Path):
        self.directory = directory

    def seems_ok(self) -> bool:
        return self.directory.exists()

    @property
    def firmware_bin(self) -> Path:
        return self.directory / 'firmware.bin'

    @property
    def firmware_elf(self) -> Path:
        return self.directory / 'firmware'

    @property
    def firmware_bbf(self) -> Path:
        return self.directory / 'firmware.bbf'

    def parse_cmake_variable(self, cmake_type, value, var_name):
        if value == '<auto>':
            raise UnsupportedOperation()
        if cmake_type == 'BOOL':
            if value.lower() in ('true', '1', 'y', 'yes', 'on'):
                value = True
            elif value.lower() in ('false', '0', 'n', 'no', 'off', ''):
                value = False
            else:
                raise ValueError(
                    f'invalid value {value} of type {cmake_type} ({var_name})')
            return value
        elif cmake_type == 'STRING':
            return value
        else:
            raise UnsupportedOperation()

    def iter_cmakecache_entries(self):
        path = self.directory / 'CMakeCache.txt'
        with open(path, 'r') as f:
            content = f.read()
            regex = r'^(\w+):(\w+)=(.*)$'
            for match in re.finditer(regex, content, re.MULTILINE):
                key, cmake_type, value = match.groups()
                try:
                    yield (key, value)
                except UnsupportedOperation:
                    pass

    def get_cmakecache_entry(self, name):
        for key, value in self.iter_cmakecache_entries():
            if key == name:
                return value
        else:
            raise KeyError(f'item {name} not found in CMakeCache')

    @staticmethod
    def find() -> Optional['BuildDirectory']:
        standard_location = BuildDirectory(project_root / 'build-vscode-buddy')
        if standard_location.seems_ok():
            return standard_location
        return None


def friendly_path(path: Path):
    path = path.absolute()
    if path.is_relative_to(Path.cwd()):
        return path.relative_to(Path.cwd())
    return path


def get_bootloader_filename(machine_type: MachineType) -> str:
    if machine_type == MachineType.MK4:
        return 'Prusa_Mk4_Boot.bin'
    elif machine_type == MachineType.XL:
        return 'Prusa_XL_Boot.bin'
    elif machine_type == MachineType.MINI:
        return 'bootloader.bin'


def make_sync(f):
    @functools.wraps(f)
    def wrapped(*args, **kwargs):
        return asyncio.run(f(*args, **kwargs))

    return wrapped


@click.command()
@click.argument('build-dir',
                type=click.Path(exists=True, path_type=Path),
                required=False)
@click.option('--bootloader-bin',
              type=click.Path(exists=True, path_type=Path),
              help='The bootloader to be used when needed.')
@click.option('--firmware',
              type=click.Path(exists=True, readable=True, path_type=Path),
              required=False)
@click.option('--printer', type=click.Choice(['MINI', 'MK4', 'XL']))
@click.option('--simulator-path',
              type=click.Path(exists=True, path_type=Path),
              default=(friendly_path(
                  get_dependency_directory('mini404') /
                  ('qemu-system-buddy.exe' if platform.system() == 'Windows'
                   else 'qemu-system-buddy'))),
              show_default=True)
@click.option('--simulator-dir',
              type=click.Path(path_type=Path),
              show_default='<BUILD_DIR>/simulator-<PRINTER>',
              help='Where to store the simulator\'s state')
@click.option('--buddy-extra-opt',
              '-X',
              multiple=True,
              help='Pass an option to the Buddy simulator')
@click.option('--xl-tools-count', type=int, default=1)
@make_sync
async def run_simulator(build_dir: Path, bootloader_bin: Path, firmware: Path,
                        printer: str, simulator_path: Path,
                        simulator_dir: Path, xl_tools_count: int,
                        buddy_extra_opt):
    """
    Runs the buddy firmware in a simulator.

    To run your localy-build firmware within the ./build-vscode-buddy build directory, just
    start it without any arguments.

        $ python utils/simulator
    """
    build_dir_cached: BuildDirectory | None = None

    def get_build_dir(reason=None):
        nonlocal build_dir_cached, build_dir
        if build_dir_cached:
            return build_dir_cached
        if build_dir:
            build_dir_cached = BuildDirectory(build_dir)
        else:
            build_dir_cached = BuildDirectory.find()
        if not build_dir_cached:
            click.echo(
                f'--build-dir not specified and no was found at the default location.'
                f' Aborting ({reason})',
                err=True)
            raise click.Abort()
        click.echo(
            f'Using build directory {click.style(build_dir_cached.directory, fg="green")}'
        )
        return build_dir_cached

    # machine type
    if not printer:
        printer = get_build_dir('--printer unknown').get_cmakecache_entry(
            'PRINTER')
    machine_type = MachineType.from_string(printer)
    click.echo(
        f'Printer: {click.style(machine_type.printer_name, fg="green")}')

    # bootloader
    if not bootloader_bin:
        cmake_bootloader = get_build_dir('bootloader?').get_cmakecache_entry(
            'BOOTLOADER')
        if cmake_bootloader.lower() in ('yes', 'y', 'empty', 'on', '1'):
            bootloader_bin = friendly_path(
                get_dependency_directory(
                    f'bootloader-{machine_type.printer_name.lower()}') /
                'bootloader.bin')
            click.secho(
                f'The firmware is configured to be run with a bootloader. Running with the default one now.',
                fg='cyan')
    click.echo(
        f'Bootloader: {click.style(bootloader_bin or "---", fg="green")}')

    # firmware
    firmware_bbf = None
    if not firmware:
        if bootloader_bin:
            firmware = get_build_dir('--firmware').firmware_bbf
        else:
            firmware = get_build_dir('--firmware').firmware_elf
        firmware_bbf = get_build_dir('--firmware').firmware_bbf
    if bootloader_bin:
        assert firmware.suffix == '.bbf', 'When running with bootloader, the --firmware has to point to a .bbf file.'
    else:
        assert firmware.suffix == '', 'When running without a bootloader, the --firmware has to point to an elf file (no suffix).'

    # simulator
    click.echo(f'Simulator: {click.style(simulator_path, fg="green")}')

    # simulator directory
    if not simulator_dir:
        simulator_dir = get_build_dir(
            '--simulator-dir'
        ).directory / f'simulator-{machine_type.printer_name.lower()}'
    simulator_dir = friendly_path(simulator_dir)
    click.echo(
        f'Simulator directory: {click.style(simulator_dir, fg="green")}')
    click.secho(
        'The USB flash drive is simulated with the `usbdir` folder within the simulator\'s directory.',
        fg='cyan')

    tmpdir = simulator_dir / 'tmp'
    usb_flash_dir = simulator_dir / 'usbdir'

    simulator_dir.mkdir(parents=True, exist_ok=True)
    usb_flash_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy(firmware, simulator_dir / firmware.name)
    if bootloader_bin:
        shutil.copy(bootloader_bin,
                    simulator_dir / get_bootloader_filename(machine_type))
    if firmware_bbf:
        shutil.copy(firmware_bbf, usb_flash_dir / firmware_bbf.name)

    def invoke_subprocess_callback(args):
        args = [shlex.quote(arg) for arg in args]
        click.secho('$ ' + ' '.join(args), fg='magenta')

    async with AsyncExitStack() as exit_stack:
        # start buddy
        buddy = await exit_stack.enter_async_context(
            Simulator.run(simulator_path=simulator_path,
                          machine=machine_type,
                          firmware_path=simulator_dir / firmware.name,
                          mount_dir_as_flash=usb_flash_dir,
                          tmpdir=tmpdir,
                          invoke_callback=invoke_subprocess_callback,
                          extra_arguments=buddy_extra_opt))

        if machine_type == MachineType.XL:
            raise NotImplementedError('XL simulator isn\'t implemented yet')
            await asyncio.sleep(2)
            async with Simulator.run(
                    simulator_path=simulator_path,
                    machine=MachineType.XL_DWARF_T0,
                    firmware_path=Path(
                        '/Users/alandragomirecky/Downloads/bootloader-v297-prusa_dwarf-1.0.elf'
                    ),
                    tmpdir=tmpdir,
                    invoke_callback=invoke_subprocess_callback):
                async with Simulator.run(
                        simulator_path=simulator_path,
                        machine=MachineType.XL_MODULARBED,
                        firmware_path=Path(
                            '/Users/alandragomirecky/Downloads/bootloader-v297-prusa_modular_bed-1.0.elf'
                        ),
                        tmpdir=tmpdir,
                        invoke_callback=invoke_subprocess_callback):

                    while True:
                        await asyncio.sleep(1200)
        else:
            if buddy.process.returncode is None:
                await buddy.process.wait()


if __name__ == '__main__':
    run_simulator()
