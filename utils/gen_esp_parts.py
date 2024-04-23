#!/usr/bin/env python3
import argparse
import hashlib
import os


def write_part(out, basedir, spec):
    basename, address = spec.split(':')
    filename = basedir + basename
    size = os.path.getsize(filename)
    raw_md5 = hashlib.md5(open(filename, 'rb').read()).hexdigest()
    raw_md5_pairs = [raw_md5[i:i + 2] for i in range(0, len(raw_md5), 2)]
    md5 = '{' + ', '.join(f"'\\x{p}'" for p in raw_md5_pairs) + '}'

    out.write('{\n')
    out.write(f'    .filename = "/internal/res/esp/{basename}",\n')
    out.write(f'    .address = {address},\n')
    out.write(f'    .size = {size},\n')
    out.write(f'    .md5 = {md5},\n')
    out.write('},\n')


def write_parts(out, basedir, name, specs):
    out.write(f'static constexpr Part {name}[] = {{\n')
    for spec in specs:
        write_part(out, basedir, spec)
    out.write('};\n')


def main():
    parser = argparse.ArgumentParser(description='Generate esp parts')
    parser.add_argument('--output')
    parser.add_argument('--basedir')
    parser.add_argument('--flash', action='append')
    parser.add_argument('--memory', action='append')
    args = parser.parse_args()

    with open(args.output, 'w') as out:
        basedir = args.basedir
        write_parts(out, basedir, 'flash_parts', args.flash or [])
        write_parts(out, basedir, 'memory_parts', args.memory or [])
        out.write('static constexpr uintptr_t memory_entry = 0x4010d004;\n')


if __name__ == '__main__':
    main()
