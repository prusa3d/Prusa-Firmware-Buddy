import os
import argparse
from pathlib import Path
import logging
import subprocess
import shutil
import tempfile

import wand
from wand.image import Image

logger = logging.getLogger('png2cc.py')


def bin2cc(src_filename: Path, dst_filename: Path, var_name: str):
    if not src_filename.exists():
        logger.error('no {} found', src_filename.name)
        return 1
    with open(src_filename.resolve(),
              "rb") as src_file, open(str(dst_filename), "w") as dst_file:
        dst_file.write("constexpr uint8_t {}[] = ".format(str(var_name)) +
                       "{\n")
        byte = src_file.read(1)
        while byte:
            dst_file.write("\t0x{},\n".format(byte.hex()))
            byte = src_file.read(1)
        dst_file.write("};\n")


def bincc_folder(src_folder: Path, dst_folder: Path):
    if not dst_folder.exists():
        logger.error('no {} found', dst_folder.name)
        return 1

    for filename in os.listdir(src_folder.resolve()):
        if filename.endswith(".png"):
            name = "png_" + filename.rstrip("png").rstrip(".")
            bin2cc((src_folder / filename).resolve(),
                   (dst_folder / (name + ".c")).resolve(), name)


def to_indexed(src_filename: Path, dst_filename: Path):
    if not src_filename.exists():
        logger.error('no {} found', src_filename.name)
        return 1

    print("{} to {}".format(src_filename, dst_filename))
    with Image(filename=src_filename.resolve()) as img:
        img.type = wand.image.IMAGE_TYPES[5]
        img.depth = 8
        img.save(filename=dst_filename.resolve())
    subprocess.run([
        "optipng -fix -o7 -zc6  -zw32k  -strip all " +
        str(dst_filename.resolve())
    ],
                   shell=True)


def to_indexed_folder(src_folder: Path, dst_folder: Path):
    if not src_folder.exists():
        logger.error('no {} found', src_folder.name)
        return 1

    for filename in os.listdir(src_folder.resolve()):
        if filename.endswith(".png"):
            to_indexed((src_folder / filename).resolve(),
                       (dst_folder / filename).resolve())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='count', default=0)

    parser.add_argument('src',
                        metavar='src',
                        type=Path,
                        help="path to png, or folder which contains pngs")
    parser.add_argument(
        '--folder',
        metavar='folder',
        type=Path,
        help="output folder for .c files, must be defined when input is folder"
    )

    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)

    if Path(args.src).is_dir() and isinstance(args.folder, type(None)):
        logger.error(
            "To convert folder of pngs the --folder flag must point to output folder"
        )
        return 1

    if Path(args.src).is_dir() and Path(args.folder).is_dir():
        tmp_name = "tmp"
        x = 0
        with tempfile.TemporaryDirectory() as tmp_dir:
            os.mkdir(tmp_dir)
            to_indexed_folder(args.src, tmp_dir)
            bincc_folder(tmp_dir, args.folder)
    elif Path(args.src).is_file():
        with tempfile.TemporaryDirectory() as tmp_dir:
            parent = Path(args.src).resolve().parent
            name = "png_" + Path(args.src).name.rstrip("png").rstrip(".")
            name_tmp = Path(tmp_dir) / Path(args.src).name
            to_indexed(args.src, name_tmp)
            bin2cc(name_tmp, parent / (name + ".c"), name)
    else:
        logger.error("mismatched input type folder and file")
        return 1


if __name__ == '__main__':
    main()
