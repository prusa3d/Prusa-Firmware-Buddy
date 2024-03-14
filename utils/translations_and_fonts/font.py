#!/usr/bin/env python3
import codecs
from pathlib import Path
import sys
import argparse
import logging
from PIL import Image
import math
import numpy as np

logger = logging.getLogger('font.py')
chars_per_row = 16
rows_to_copy = 6


def remove_red_dots(image: np.array) -> np.array:
    for i in range(image.shape[0]):
        for j in range(image.shape[1]):
            if image[i, j, 0] == 255 and image[i, j, 1] == 0 and image[i, j,
                                                                       2] == 0:
                image[i, j] = np.array([255, 255, 255])
    return image


def cmd_create_font_png(non_ascii_chars_path: Path, src_png_path: Path,
                        char_width: int, char_height: int, ipp_path: Path,
                        dst_png_path: Path):
    if not non_ascii_chars_path.exists():
        logger.error('no %s found', non_ascii_chars_path.name)
        return 1
    if not src_png_path.exists():
        logger.error('no %s found', src_png_path.name)
        return 1

    # not using with because I want to use charset outside of its scope

    with open(non_ascii_chars_path.resolve()) as file:
        chars = file.read()  # discards 0xa0 non-breaking space character
        char_set = chars.split()

    with Image.open(src_png_path.resolve()) as src_png:
        src_png_mode = src_png.mode
        if src_png_mode != "RGB":
            logger.error('%s mode is %s instead of required RGB', src_png_path,
                         src_png_mode)
        num_of_rows = rows_to_copy + math.ceil(len(char_set) / chars_per_row)
        output_image = Image.new(
            src_png_mode,
            (chars_per_row * char_width, num_of_rows * char_height),
            color="white")

        # copy standard ASCII characters
        for x in range(chars_per_row):
            for y in range(rows_to_copy):
                char = src_png.crop(
                    (x * char_width, y * char_height, (x + 1) * char_width,
                     (y + 1) * char_height))
                output_image.paste(char, (x * char_width, y * char_height,
                                          (x + 1) * char_width,
                                          (y + 1) * char_height))

        x = 0
        y = rows_to_copy
        print("IPP", ipp_path)
        with open(str(ipp_path.resolve()), "w") as file:
            file.write("{\n")
            for char in char_set:
                char_bytes = char.encode("utf-32", "little")
                if char_bytes.startswith(codecs.BOM_UTF32):
                    char_bytes = char_bytes[len(codecs.BOM_UTF32):]
                # char_bytes = char_bytes.removeprefix(codecs.BOM_UTF32) # New in version 3.9.
                char_int = int.from_bytes(char_bytes, "little")
                if char_int < 128:
                    continue
                char_int -= 32  # this is where out bitmap starts

                srcX = char_int % chars_per_row
                srcY = char_int // chars_per_row
                char = src_png.crop(
                    (srcX * char_width, srcY * char_height,
                     (srcX + 1) * char_width, (srcY + 1) * char_height))
                output_image.paste(char, (x * char_width, y * char_height,
                                          (x + 1) * char_width,
                                          (y + 1) * char_height))
                file.write("{")
                file.write(" {}, {}, {}".format(
                    hex(int.from_bytes(char_bytes, "little")), x, y))
                file.write("},\n")

                x += 1
                if (x >= chars_per_row):
                    x = 0
                    y += 1
            file.write("};\n")

        image = remove_red_dots(np.array(output_image, dtype=np.uint8))
        output_image = Image.fromarray(image, "RGB")
        output_image.save(dst_png_path.resolve())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='count', default=0)

    parser.add_argument('non_ascii_chars',
                        metavar='non_ascii_chars',
                        type=Path)
    parser.add_argument('src_png', metavar='src_png', type=Path)
    parser.add_argument('char_width', metavar='char_width', type=int)
    parser.add_argument('char_height', metavar='char_height', type=int)
    parser.add_argument('dst_png', metavar='dst_png', type=Path)
    parser.add_argument('ipp_path', metavar='ipp_path', type=Path)

    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    retval = cmd_create_font_png(args.non_ascii_chars, args.src_png,
                                 args.char_width, args.char_height,
                                 args.ipp_path, args.dst_png)
    sys.exit(retval if retval is not None else 0)


if __name__ == '__main__':
    main()
