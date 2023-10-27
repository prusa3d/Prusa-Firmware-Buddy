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
import platform
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
            'Windows': 'https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-win.zip',
            'Darwin': 'https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-mac.zip',
        },
    },
    'cmake': {
        'version': '3.22.5',
        'url': {
            'Linux': 'https://github.com/Kitware/CMake/releases/download/v3.22.5/cmake-3.22.5-linux-x86_64.tar.gz',
            'Windows': 'https://github.com/Kitware/CMake/releases/download/v3.22.5/cmake-3.22.5-windows-x86_64.zip',
            'Darwin': 'https://github.com/Kitware/CMake/releases/download/v3.22.5/cmake-3.22.5-macos-universal.tar.gz',
        },
    },
    'gcc-arm-none-eabi': {
        'version': '10.3.1',
        'url': {
            'Linux': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2',
            'Windows': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip',
            'Darwin': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2',
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
        'version': '2.3.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mini-2.3.0-B6680FEB-C7C1-438B-B297-77FA012CB9FE.zip',
    },
    'bootloader-mk4': {
        'version': '2.3.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.3.0-A98CD828-A5FA-48C6-BE76-3BA986BC2F0C.zip',
    },
    'bootloader-mk3.5': {
        'version': '2.3.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.3.0-A98CD828-A5FA-48C6-BE76-3BA986BC2F0C.zip',
    },
    'bootloader-xl': {
        'version': '2.3.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-xl-2.3.0-40D68D78-E58B-434E-9237-CC58CB2E7F1C.zip',
    },
    'bootloader-ix': {
        'version': '2.3.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-ix-2.3.0-0BBB99FE-6992-4A56-8A7C-3729EEDB5A8F.zip',
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
        'version': '0.4.9999',
        'url': 'https://github.com/posborne/cmsis-svd/archive/45a1e90afe488f01df94b3e0eb89a67c1a900a9a.zip',
    },
    'CrashDebug': {
        'version': 'ae191d',
        'url': 'https://github.com/adamgreen/CrashDebug/archive/ae191d7ad96258570d5e6685619cab31c0d3859c.zip',
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
    url = specs['url']
    if isinstance(url, dict):
        url = url[platform.system()]
    download_and_unzip(url=url, directory=installation_directory)
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
