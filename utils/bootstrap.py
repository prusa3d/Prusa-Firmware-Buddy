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
import zipfile
from argparse import ArgumentParser
from pathlib import Path
from urllib.request import urlretrieve

project_root_dir = Path(__file__).resolve().parent.parent
dependencies_dir = project_root_dir / '.dependencies'

# All dependencies of this project.
#
# yapf: disable
dependencies = {
    'ninja': {
        'version': '1.9.0',
        'url': {
            'Linux': 'https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip',
            'Windows': 'https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-win.zip',
            'Darwin': 'https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-mac.zip',
        },
    },
    'cmake': {
        'version': '3.15.5',
        'url': {
            'Linux': 'https://github.com/Kitware/CMake/releases/download/v3.15.5/cmake-3.15.5-Linux-x86_64.tar.gz',
            'Windows': 'https://github.com/Kitware/CMake/releases/download/v3.15.5/cmake-3.15.5-win64-x64.zip',
            'Darwin': 'https://github.com/Kitware/CMake/releases/download/v3.15.5/cmake-3.15.5-Darwin-x86_64.tar.gz',
        },
    },
    'gcc-arm-none-eabi': {
        'version': '7.3.1',
        'url': {
            'Linux': 'https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2',
            'Windows': 'https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-win32.zip',
            'Darwin': 'https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2',
        }
    },
    'clang-format': {
        'version': '9.0.0-noext',
        'url': {
            'Linux': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/clang-format-9.0.0-linux.zip',
            'Windows': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/clang-format-9.0.0-noext-win.zip',
            'Darwin': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/clang-format-9.0.0-darwin.zip',
        }
    },
    'bootloader-mini': {
        'version': '1.0.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mini-1.0.0.zip',
    },
}
pip_dependencies = ['ecdsa']
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


def initialize_submodule(path):
    cmd = [
        'git', '-C',
        str(project_root_dir), 'submodule', 'update', '--init', '--', path
    ]
    subprocess.run(cmd, check=True, encoding='utf-8')


def check_submodules():
    cmd = ['git', '-C', str(project_root_dir), 'submodule', 'status']
    try:
        process = subprocess.run(cmd,
                                 stdout=subprocess.PIPE,
                                 encoding='utf-8',
                                 check=True)
    except (subprocess.SubprocessError, FileNotFoundError):
        msg = 'WARNING: Failed to check submodules status'
        print(msg, file=sys.stderr)
        return
    statuses = [line.split() for line in process.stdout.splitlines()]
    for status in statuses:
        if status[0].startswith('-'):  #  '-' means uninitialized
            print('Submodule %s is not initiliazed. Initializing now...' %
                  status[1])
            initialize_submodule(status[1])
        elif status[0][0] in ('U', '+'):
            print('WARNING: Submodule %s does not seem to be up-to-date!'
                  ' Consider running `git submodule update`.' % status[1])


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
            version = dependencies[args.print_dependency_version]['version']
            print(version)
            return 0
        except KeyError:
            print('Unknown dependency "%s"' % args.print_dependency_version)
            return 1

    if args.print_dependency_directory:
        try:
            dependency = args.print_dependency_directory
            version = dependencies[dependency]['version']
            install_dir = directory_for_dependency(dependency, version)
            print(install_dir)
            return 0
        except KeyError:
            print('Unknown dependency "%s"' % args.print_dependency_directory)
            return 1

    # if no argument present, check and install dependencies
    for dependency in dependencies:
        if recommended_version_is_available(dependency):
            continue
        install_dependency(dependency)

    # also, install pip packages
    installed_pip_packages = get_installed_pip_packages()
    for package in pip_dependencies:
        is_installed = any(installed[0] == package
                           for installed in installed_pip_packages)
        if is_installed:
            continue
        print('Installing Python package %s' % package)
        run(sys.executable, '-m', 'pip', 'install', package,
            '--disable-pip-version-check')

    # check submodules are initialized
    check_submodules()

    return 0


if __name__ == "__main__":
    sys.exit(main())
