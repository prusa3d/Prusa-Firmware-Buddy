#!/usr/bin/env python3
#
# Bootstrap Script
#
# This script
#  1) records the recommended versions of dependencies, and
#  2) when run, checks that all of them are present and downloads
#       them if they are not.
#
# pylint: disable=line-too-long
import json
import os
import platform
import shutil
import stat
import subprocess
import sys
import tarfile
import venv
import zipfile
import stat
from argparse import ArgumentParser
from pathlib import Path
from urllib.request import urlretrieve

assert sys.version_info >= (3, 8), 'Python 3.8+ is required.'
is_windows = platform.system() == 'Windows'
project_root_dir = Path(__file__).resolve().parent.parent
dependencies_dir = project_root_dir / '.dependencies'
venv_dir = project_root_dir / '.venv'
venv_bin_dir = venv_dir / 'bin' if not is_windows else venv_dir / 'Scripts'
running_in_venv = Path(sys.prefix).resolve() == venv_dir.resolve()

# All dependencies of this project.
#
# yapf: disable
dependencies = {
    'ninja': {
        'version': '1.10.2',
        'url': {
            'Linux': 'https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip',
            'Linux-aarch64': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/ninja-v1.10.2-linux-aarch64.zip',
            'Windows': 'https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-win.zip',
            'Darwin': 'https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-mac.zip',
        },
    },
    'cmake': {
        'version': '3.30.3',
        'url': {
            'Linux': 'https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-linux-x86_64.tar.gz',
            'Linux-aarch64': 'https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-linux-aarch64.tar.gz',
            'Windows': 'https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-windows-x86_64.zip',
            'Windows-arm64': 'https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-windows-arm64.zip',
            'Darwin': 'https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-macos-universal.tar.gz',
        },
    },
    'gcc-arm-none-eabi': {
        'version': '13.2.1',
        'url': {
            'Linux': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz',
            'Linux-aarch64': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-aarch64-arm-none-eabi.tar.xz',
            'Windows': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-mingw-w64-i686-arm-none-eabi.zip',
            'Darwin': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-darwin-x86_64-arm-none-eabi.tar.xzg',
        }
    },
    'clang-format': {
        'version': '16-83817c2f',
        'url': {
            'Linux': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/clang-format-16-83817c2f-linux.zip',
            'Windows': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/clang-format-16-83817c2f-windows.zip',
            'Darwin': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/clang-format-16-83817c2f-macosx.zip',
        }
    },
    'bootloader-mini': {
        'version': '2.3.5',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mini-2.3.5-3B618C23-C32F-4B7B-BBAB-050E3A3FC7D0.zip',
    },
    'bootloader-mk4': {
        'version': '2.3.5',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.3.5-11516FDF-4631-4AF6-A161-1A91BDAB4A2B.zip',
    },
    'bootloader-mk3.5': {
        'version': '2.3.5',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.3.5-11516FDF-4631-4AF6-A161-1A91BDAB4A2B.zip',
    },
    'bootloader-xl': {
        'version': '2.3.5',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-xl-2.3.5-46A7B599-F91D-4380-A4DE-C6A1BAE79234.zip',
    },
    'bootloader-ix': {
        'version': '2.3.5',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-ix-2.3.5-3EF50AB4-ECC7-472E-B10B-777E07955517.zip',
    },
    'firmware-mmu': {
        'version': '3.0.3',
        'files': [
            # 'https://github.com/prusa3d/Prusa-Firmware-MMU/releases/download/v3.0.2/MMU2S_MMU3_FW3.0.2+878.hex',
            # 3.0.3 published *privately* because it's just a very minor change
            'https://download.danol.cz/MMU2S_MMU3_FW3.0.3+895.hex'
        ],
    },
    'mini404': {
        'version': '0.9.10',
        'url': {
            'Linux': 'https://github.com/vintagepc/MINI404/releases/download/v0.9.10/Mini404-v0.9.10-linux.tar.bz2',
            'Windows': 'https://github.com/vintagepc/MINI404/releases/download/v0.9.10/Mini404-v0.9.10-w64.zip',
            'Darwin': 'https://github.com/vintagepc/MINI404/releases/download/v0.9.10/Mini404-v0.9.10-macos.tar.bz2',
        }
    },
    'cmsis-svd': {
        'version': '40327a4d2dff0992682be2872aaa6e096f35d2f4',
        'files': [
            'https://raw.githubusercontent.com/cmsis-svd/cmsis-svd-data/40327a4d2dff0992682be2872aaa6e096f35d2f4/data/STMicro/STM32F427.svd',
            'https://raw.githubusercontent.com/cmsis-svd/cmsis-svd-data/40327a4d2dff0992682be2872aaa6e096f35d2f4/data/STMicro/STM32G07x.svd',
        ],
    },
    'CrashDebug': {
        'version': 'ab03f8b6fb6e3445c62fe3fa5f3263d2945d74ff',
        'url': 'https://github.com/prusa3d/CrashDebug/archive/ab03f8b6fb6e3445c62fe3fa5f3263d2945d74ff.zip',
    },
}
# yapf: enable


def directory_for_dependency(dependency, version):
    return dependencies_dir / (dependency + '-' + version)


def find_single_subdir(path: Path):
    members = list(path.iterdir())
    if path.is_dir() and len(members) > 1:
        return path
    elif path.is_dir() and len(members) == 1:
        return find_single_subdir(members[0]) if members[0].is_dir() else path
    else:
        raise RuntimeError


def download_and_unzip(url: str, directory: Path):
    """Download a compressed file and extract it at `directory`."""
    extract_dir = directory.with_suffix('.temp')
    shutil.rmtree(directory, ignore_errors=True)
    shutil.rmtree(extract_dir, ignore_errors=True)

    print('Downloading ' + directory.name)
    f, _ = urlretrieve(url, filename=None)
    print('Extracting ' + directory.name)
    if '.tar.bz2' in url or '.tar.gz' in url or '.tar.xz' in url:
        obj = tarfile.open(f)
    else:
        obj = zipfile.ZipFile(f, 'r')
    obj.extractall(path=str(extract_dir))

    subdir = find_single_subdir(extract_dir)
    shutil.move(str(subdir), str(directory))
    shutil.rmtree(extract_dir, ignore_errors=True)


def run(*cmd):
    process = subprocess.run([str(a) for a in cmd],
                             stdout=subprocess.PIPE,
                             check=True,
                             encoding='utf-8')
    return process.stdout.strip()


def fix_executable_permissions(dependency, installation_directory):
    to_fix = ('ninja', 'clang-format')
    if dependency not in to_fix:
        return
    for fpath in installation_directory.iterdir():
        if fpath.is_file and fpath.with_suffix('').name in to_fix:
            st = os.stat(fpath)
            os.chmod(fpath, st.st_mode | stat.S_IEXEC)


def recommended_version_is_available(dependency):
    version = dependencies[dependency]['version']
    directory = directory_for_dependency(dependency, version)
    return directory.exists() and directory.is_dir()


def get_installed_pip_packages():
    result = run(sys.executable, '-m', 'pip', 'list',
                 '--disable-pip-version-check', '--format', 'json')
    data = json.loads(result)
    return [(pkg['name'].lower(), pkg['version']) for pkg in data]


def install_dependency(dependency):
    specs = dependencies[dependency]
    installation_directory = directory_for_dependency(dependency,
                                                      specs['version'])
    url = specs.get('url', None)
    files = specs.get('files', None)
    if url is not None:
        if isinstance(url, dict):
            full_description = f'{platform.system()}-{platform.machine()}'
            if full_description not in url:
                url = url[platform.system()]
            else:
                url = url[full_description]
        download_and_unzip(url=url, directory=installation_directory)
    elif files is not None:
        os.mkdir(installation_directory)
        for file in files:
            basename = file.split('/')[-1]
            urlretrieve(file, installation_directory / basename)
    else:
        raise ('dependency is missing payload')

    fix_executable_permissions(dependency, installation_directory)


def install_openocd_config_template():
    debug_dir = project_root_dir / 'utils' / 'debug'
    custom_config_path = debug_dir / '10_custom_config.cfg'
    custom_config_template_path = debug_dir / '10_custom_config_template.cfg'
    if not custom_config_path.exists():
        print(f'Installing openocd user-config to {custom_config_path}')
        shutil.copy(custom_config_template_path, custom_config_path)


def get_dependency_version(dependency):
    return dependencies[dependency]['version']


def get_dependency_directory(dependency) -> Path:
    version = dependencies[dependency]['version']
    return Path(directory_for_dependency(dependency, version))


def switch_to_venv_if_nedded():
    if not running_in_venv and os.environ.get('BUDDY_NO_VIRTUALENV') != '1':
        if not os.path.exists(".venv"):
            print('Creating needed virtual environment in .venv')
            os.system(sys.executable + ' -m venv .venv')
        print('Switching to Buddy\'s virtual environment.', file=sys.stderr)
        print(
            'You can disable this by setting the BUDDY_NO_VIRTUALENV=1 env. variable.',
            file=sys.stderr)
        os.execv(str(venv_bin_dir / 'python'),
                 [str(venv_bin_dir / 'python')] + sys.argv)


def prepare_venv_if_needed():
    if venv_dir.exists():
        return
    venv.create(venv_dir, with_pip=True, prompt='buddy')


def pip_install(*args):
    command = [
        str(venv_bin_dir / 'python'), '-m', 'pip', 'install',
        '--disable-pip-version-check', '--no-input', *args
    ]
    process = subprocess.Popen(command,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               encoding='utf-8')
    assert process.stdout is not None
    for line in iter(process.stdout.readline, ''):
        if line.lower().startswith('requirement already satisfied:'):
            continue
        if line.lower().startswith('looking in indexes'):
            continue
        print(line.rstrip(), file=sys.stderr)
    process.communicate()
    if process.returncode != 0:
        sys.exit(process.returncode)


def install_pip_packages():
    requirements_path = project_root_dir / 'requirements.txt'
    # find required pip and install it first
    with open(requirements_path, 'r') as f:
        for line in f:
            if line.startswith('pip'):
                pip_install(line.strip())
                break
        else:
            raise RuntimeError('pip not found in requirements.txt')
    # install everything else from requirements.txt
    pip_install('-r', str(requirements_path))


def bootstrap():
    for dependency in dependencies:
        if recommended_version_is_available(dependency):
            continue
        install_dependency(dependency)

    prepare_venv_if_needed()
    install_pip_packages()

    # also, install openocd config meant for customization
    install_openocd_config_template()


def main() -> int:
    parser = ArgumentParser()
    # yapf: disable
    parser.add_argument(
        '--print-dependency-version', type=str,
        help='Prints recommended version of given dependency and exits.')
    parser.add_argument(
        '--print-dependency-directory', type=str,
        help='Prints installation directory of given dependency and exits.')
    args = parser.parse_args(sys.argv[1:])
    # yapf: enable

    if args.print_dependency_version:
        try:
            print(get_dependency_version(args.print_dependency_version))
            return 0
        except KeyError:
            print('Unknown dependency "%s"' % args.print_dependency_version)
            return 1

    if args.print_dependency_directory:
        try:
            print(get_dependency_directory(args.print_dependency_directory))
            return 0
        except KeyError:
            print('Unknown dependency "%s"' % args.print_dependency_directory)
            return 1

    # if no argument present, check and install dependencies
    bootstrap()

    return 0


if __name__ == "__main__":
    sys.exit(main())
