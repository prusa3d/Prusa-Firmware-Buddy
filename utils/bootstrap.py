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
from urllib.parse import urlparse

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
        'version': '3.28.3',
        'url': {
            'Linux': 'https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.tar.gz',
            'Linux-aarch64': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/cmake-3.28.3-Linux-aarch64.tar.gz',
            'Windows': 'https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-windows-x86_64.zip',
            'Darwin': 'https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-macos-universal.tar.gz',
        },
    },
    'gcc-arm-none-eabi': {
        'version': '13.3.1',
        'url': {
            'Linux': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz',
            'Linux-aarch64': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-aarch64-arm-none-eabi.tar.xz',
            'Windows': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-mingw-w64-i686-arm-none-eabi.zip',
            'Darwin': 'https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-darwin-x86_64-arm-none-eabi.tar.xz',
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
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mini-2.6.0-3F576FB7-DF99-4877-AB20-BF3CA45AD1FC.zip',
    },
    'bootloader-mk4': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.6.0-48C7820D-1C5F-44D5-A4D4-B4C0F8CCB2A6.zip',
    },
    'bootloader-coreone': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.6.0-48C7820D-1C5F-44D5-A4D4-B4C0F8CCB2A6.zip',
    },
    'bootloader-coreone_indx': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.6.0-48C7820D-1C5F-44D5-A4D4-B4C0F8CCB2A6.zip',
    },
    'bootloader-coreonel': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.6.0-48C7820D-1C5F-44D5-A4D4-B4C0F8CCB2A6.zip',
    },
    'bootloader-coreonel_indx': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.6.0-48C7820D-1C5F-44D5-A4D4-B4C0F8CCB2A6.zip',
    },
    'bootloader-indx_head': {
        'version': 'v303',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-indx_head-v303-8B50471A-8B26-4941-8C05-68E364C27DF5.zip',
    },
    'bootloader-mk3.5': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-mk4-2.6.0-48C7820D-1C5F-44D5-A4D4-B4C0F8CCB2A6.zip',
    },
    'bootloader-xl': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-xl-2.6.0-BBF9BE22-2A89-4B82-AAFA-A463F8AD2ABF.zip',
    },
    'bootloader-ix': {
        'version': '2.6.0',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/bootloader-ix-2.6.0-36BB9EB6-4DE5-4007-B635-EDD577CBB3BA.zip',
    },
    'firmware-coreonel_ac_controller': {
        'version': '0.0.8',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/firmware-coreonel_ac_controller-0.0.8-ECD94653-E201-4B7C-A9D4-1439BF25D7E5.zip',
    },
    'firmware-coreonel_indx_ac_controller': {
        'version': '0.0.8',
        'url': 'https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/firmware-coreonel_ac_controller-0.0.8-ECD94653-E201-4B7C-A9D4-1439BF25D7E5.zip',
    },
    'firmware-mmu': {
        'version': '3.0.4',
        'files': [
            'https://github.com/prusa3d/Prusa-Firmware-MMU/releases/download/v3.0.4/MMU3_FW3.0.4+936.hex'
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
    'cmsis-svd': {
        'version': 'c7e833e81e79e3396e1078832d936ae6578a8bc1',
        'files': [
            'https://raw.githubusercontent.com/modm-io/cmsis-svd-stm32/c7e833e81e79e3396e1078832d936ae6578a8bc1/stm32h5/STM32H503.svd',
        ],
    },
    'cmsis-svd-c0': {
        'version': 'ce4b873ae4a471cf28d7a7c9c69ded252d949672',
        'files': [
            'https://raw.githubusercontent.com/modm-io/cmsis-svd-stm32/ce4b873ae4a471cf28d7a7c9c69ded252d949672/stm32c0/STM32C092.svd',
        ],
    },
    'CrashDebug': {
        'version': '22acef8c6e248db2f04f65eebf4c2b470f4010c2',
        'url': 'https://github.com/prusa3d/CrashDebug/archive/22acef8c6e248db2f04f65eebf4c2b470f4010c2.zip',
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


def download_url(url: str, filename: Path):
    """Download file from url and write it to given filename"""
    import requests
    with requests.get(url, stream=True) as response:
        response.raise_for_status()
        with open(filename, 'wb') as file:
            for chunk in response.iter_content(chunk_size=8192):
                if chunk:
                    file.write(chunk)


def download_and_unzip(url: str, directory: Path):
    """Download a compressed file and extract it at `directory`."""
    extract_dir = directory.with_suffix('.temp')
    shutil.rmtree(directory, ignore_errors=True)
    shutil.rmtree(extract_dir, ignore_errors=True)

    print('Downloading ' + directory.name, end=" ")

    # temporary local filepath
    parsed_url = urlparse(url)
    file = Path(parsed_url.path).name
    filename = Path(dependencies_dir) / file

    download_url(url=url, filename=filename)

    print('done')
    print('Extracting ' + file, end=" ")

    # Check if tar or zip
    if any(
            url.endswith(ext)
            for ext in ['.tar.bz2', '.tar.gz', '.tar.xz', 'tar.xzg']):
        with tarfile.open(filename) as obj:
            obj.extractall(path=extract_dir)
    else:
        with zipfile.ZipFile(filename, 'r') as obj:
            obj.extractall(path=extract_dir)

    subdir = find_single_subdir(extract_dir)
    shutil.move(subdir, directory)

    print('done')

    # remove temp unzip folder
    shutil.rmtree(extract_dir, ignore_errors=True)

    # remove downloaded zip
    os.remove(filename)


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
            print('Downloading ' + file)
            download_url(url=file, filename=installation_directory / basename)
    else:
        raise ('dependency is missing payload')

    fix_executable_permissions(dependency, installation_directory)


def install_openocd_config_template():
    debug_dir = project_root_dir / 'utils' / 'debug'
    os.makedirs(debug_dir, exist_ok=True)
    custom_config_path = debug_dir / 'device_setup_overrides.cfg'
    if not custom_config_path.exists():
        print(
            f'Installing openocd user-config override to {custom_config_path}')
        custom_config_path.write_text(
            "# This file is meant for custom configuration overrides.\n# See 10_device_setup.cfg for info and copy one proc section here.\n"
        )


def install_git_hooks():
    # Pin prek to the root config (single-config mode), matching old pre-commit.
    if not (project_root_dir / '.git').exists():
        return
    try:
        subprocess.run([
            str(venv_bin_dir / 'prek'), 'install', '--overwrite', '-c',
            '.pre-commit-config.yaml'
        ],
                       cwd=str(project_root_dir),
                       check=True)
    except (subprocess.CalledProcessError, OSError) as exc:
        print(f'Warning: failed to install prek hooks: {exc}', file=sys.stderr)


def get_dependency_version(dependency):
    return dependencies[dependency]['version']


def get_dependency_directory(dependency) -> Path:
    version = dependencies[dependency]['version']
    return Path(directory_for_dependency(dependency, version))


def switch_to_venv_if_needed():
    if not running_in_venv and os.environ.get('BUDDY_NO_VIRTUALENV') != '1':
        prepare_venv_if_needed(include_integration=False)
        print('Switching to Buddy\'s virtual environment.', file=sys.stderr)
        print(
            'You can disable this by setting the BUDDY_NO_VIRTUALENV=1 env. variable.',
            file=sys.stderr)
        os.execv(str(venv_bin_dir / 'python'),
                 [str(venv_bin_dir / 'python')] + sys.argv)


def prepare_venv_if_needed(include_integration):
    if venv_dir.exists():
        return
    venv.create(venv_dir, with_pip=True, prompt='buddy')
    install_pip_packages(include_integration)


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


def install_pip_packages(include_integration):
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

    if include_integration:
        integration_requirements_path = project_root_dir / 'integration-test-requirements.txt'
        pip_install('-r', str(integration_requirements_path))


def bootstrap(include_integration):
    # create dependency directory if not exists
    if not os.path.exists(dependencies_dir):
        os.makedirs(dependencies_dir)

    for dependency in dependencies:
        if recommended_version_is_available(dependency):
            continue
        install_dependency(dependency)

    prepare_venv_if_needed(include_integration)
    install_pip_packages(include_integration)

    # also, install openocd config meant for customization
    install_openocd_config_template()

    # install prek Git hooks
    install_git_hooks()


def main() -> int:
    parser = ArgumentParser()
    # yapf: disable
    parser.add_argument(
        '--print-dependency-version', type=str,
        help='Prints recommended version of given dependency and exits.')
    parser.add_argument(
        '--print-dependency-directory', type=str,
        help='Prints installation directory of given dependency and exits.')
    parser.add_argument(
        '--include-integration', action='store_true',
        help='Also install integration test dependencies (easyocr, etc.).')
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
    bootstrap(include_integration=args.include_integration)

    return 0


if __name__ == "__main__":
    sys.exit(main())
