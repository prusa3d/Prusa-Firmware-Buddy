#!/usr/bin/env python3
from argparse import ArgumentParser
import os
from pathlib import Path
from PIL import Image
import qoi
import numpy


def main():
    parser = ArgumentParser()
    parser.add_argument(
        'input',
        type=Path,
        help='Path to directory with PNG images or a single image')
    parser.add_argument('resources_file',
                        type=Path,
                        help='Path to file where to store QOI resources file')
    parser.add_argument('data_file',
                        type=Path,
                        help='Path to file where to create QOI data file')
    parser.add_argument(
        'input_filter',
        type=Path,
        help='Path to file with list of actually used images in this printer')

    args = parser.parse_args()

    png_files = []
    if os.path.isdir(args.input.resolve()):
        # Sort for reproducible builds: os.listdir() returns entries in
        # filesystem-dependent order, which makes the packed qoi.data blob and
        # the generated Resource offsets differ between build hosts.
        for filename in sorted(os.listdir(args.input.resolve())):
            if filename.endswith('.png'):
                png_files.append((args.input.resolve() / filename).resolve())
    else:
        png_files.append(args.input.resolve())

    filtered_pngs = []
    with open(args.input_filter.resolve(), 'r') as fd:
        for line in fd:
            filtered_pngs.append(line.strip())

    offset = 0
    with open(args.resources_file, 'w') as resources_file, \
         open(args.data_file, 'wb+') as data_file:

        for png_file in png_files:
            with Image.open(png_file.resolve()).convert('RGBA') as png:
                name = png_file.name.split('.')[0]
                width = png.width
                height = png.height
                data = qoi.encode(numpy.array(png))
                size = len(data)

                if name + ".png" in filtered_pngs:
                    resources_file.write(
                        f'inline constexpr Resource {name}({offset}, {width}, {height});\n'
                    )
                    data_file.write(data)
                    offset = offset + size
                else:
                    resources_file.write(f'extern Resource {name};\n')


if __name__ == '__main__':
    main()
