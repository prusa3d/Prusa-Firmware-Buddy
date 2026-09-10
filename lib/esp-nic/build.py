#!/usr/bin/env python3
#
# Builds this firmware.
# Expects IDF_PATH to point at the SDK with its tools installed, and cmake and ninja on PATH.
import os
import subprocess
import time
from argparse import ArgumentParser
from pathlib import Path

project_dir = Path(__file__).resolve().parent


def is_reproducible(build_dir: Path):
    forbidden = [
        str(Path.home()), os.environ['IDF_PATH'],
        str(build_dir),
        str(project_dir),
        time.strftime('%b %e %Y')
    ]
    for artifact in [*build_dir.rglob('*.elf'), *build_dir.rglob('*.bin')]:
        data = artifact.read_bytes()
        for path in forbidden:
            if path.encode() in data:
                return False
    return True


def prefix_map_flags(old, new):
    return f'-ffile-prefix-map={old}={new} -Wa,--debug-prefix-map={old}={new}'


def build(variant: str, build_dir: Path):
    sdk_dir = os.environ.get('IDF_PATH')
    assert (sdk_dir)

    prefix_map = ' '.join([
        prefix_map_flags(sdk_dir, '/sdk'),
        prefix_map_flags(str(build_dir), '/build'),
        prefix_map_flags(str(project_dir), '/project')
    ])

    env = dict(os.environ,
               BUILD_DIR=str(build_dir),
               SDKCONFIG=str(project_dir / f'{variant}.sdkconfig'),
               EXTRA_CPPFLAGS=prefix_map,
               CFLAGS=prefix_map,
               ASMFLAGS=prefix_map,
               SOURCE_DATE_EPOCH='0',
               PYTHONHASHSEED='0',
               PYTHONWARNINGS='ignore')
    subprocess.run([
        'bash', '-c',
        '. "$IDF_PATH/export.sh" > /dev/null && exec idf.py -B "$BUILD_DIR" '
        '-DSDKCONFIG="$SDKCONFIG" -DEXCLUDE_COMPONENTS="$EXCLUDE_COMPONENTS" '
        '-DCMAKE_MESSAGE_LOG_LEVEL=NOTICE all'
    ],
                   cwd=str(project_dir),
                   env=env,
                   check=True)

    assert (is_reproducible(build_dir))


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('variant', choices=['esp8266', 'esp32'])
    parser.add_argument('--build-dir',
                        type=Path,
                        default=project_dir / 'build')
    args = parser.parse_args()
    build(args.variant, args.build_dir)
