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


def remove_red_dots(image: np.array) -> np.array:
    for i in range(image.shape[0]):
        for j in range(image.shape[1]):
            if image[i, j, 0] == 255 and image[i, j, 1] == 0 and image[i, j,
                                                                       2] == 0:
                image[i, j] = np.array([255, 255, 255])
    return image


def char_to_int(char) -> int:
    char_bytes = char.encode("utf-32", "little")
    if char_bytes.startswith(codecs.BOM_UTF32):
        char_bytes = char_bytes[len(codecs.BOM_UTF32):]
    # char_bytes = char_bytes.removeprefix(codecs.BOM_UTF32) # New in version 3.9.
    return int.from_bytes(char_bytes, "little")


# based on charset_option ("full" / "standard" / "digits") different paths will be used
def cmd_create_font_png(charset_option: str, required_chars_path: Path,
                        src_png_path: Path, src_png_jap_path: Path,
                        char_width: int, char_height: int, ipp_path: Path,
                        dst_png_path: Path):
    if not required_chars_path.exists():
        logger.error('no %s found', required_chars_path)
        return 1
    if not src_png_path.exists():
        logger.error('no %s found', src_png_path)
        return 1
    if not src_png_jap_path.exists(
    ):  # Katakana source png is mandatory for every font
        logger.error('no %s found', src_png_jap_path)
        return 1

    # Extract current character set
    char_list = []
    with open(required_chars_path.resolve()) as file:
        chars = file.read()
        char_list = chars.split()

    if not char_list:
        logger.error(
            'font.py::cmd_create_font_png: unsupported charset_option %s or unable to open required_chars_path',
            charset_option)
        return 1

    # Add space back in the set
    char_list.append(' ')
    char_list = sorted(char_list)
    fail = False

    with Image.open(src_png_path.resolve()) as src_lat_png:
        with Image.open(src_png_jap_path.resolve()) as src_jap_png:
            # Some font have no need for japanese at all, but I leave it here just in case its char set is changed and japanese characters are needed
            # Each font have to have its own katakana alphabet anyway
            src_lat_png_mode = src_lat_png.mode
            src_jap_png_mode = src_jap_png.mode
            if src_lat_png_mode != "RGB":
                logger.error('LATIN: %s mode is %s instead of required RGB',
                             src_png_path, src_lat_png_mode)
            if src_jap_png_mode != "RGB":
                logger.error('KATAKANA: %s mode is %s instead of required RGB',
                             src_png_jap_path, src_jap_png_mode)

            num_of_rows = math.ceil(len(char_list) / chars_per_row)
            output_image = Image.new(
                src_lat_png_mode,
                (chars_per_row * char_width, num_of_rows * char_height),
                color="white")

            print("IPP:", ipp_path)

            x = 0
            y = 0
            with open(str(ipp_path.resolve()), "w") as file:
                for ch in char_list:
                    # JAPANESE
                    if (ord(ch) >= 0x30A0
                            and ord(ch) <= 0x30FF) or ch == '、' or ch == '。':
                        # SPECIAL CASES (comma and dot are appended to katakana fonts)
                        if ch == '、' or ch == '。':
                            # Hardcoded coordinates in our katakana font source png
                            srcX = 0 if ch == '、' else 1
                            srcY = 6
                        else:
                            # KATAKANA
                            katakana_index = (ord(ch) - 0x30A0)
                            srcX = katakana_index % chars_per_row
                            srcY = katakana_index // chars_per_row
                        src_png = src_jap_png

                    else:
                        char_int = char_to_int(ch)
                        char_int -= 32  # this is where out standard ASCII bitmap starts
                        srcX = char_int % chars_per_row
                        srcY = char_int // chars_per_row
                        src_png = src_lat_png

                    x_max, y_max = src_png.size
                    if (srcY + 1) * char_height > y_max or char_int < 0:
                        # Unsupported character
                        logger.error("Unsupported character found: \"%c\"", ch)
                        fail = True
                        continue

                    char_crop = src_png.crop(
                        (srcX * char_width, srcY * char_height,
                         (srcX + 1) * char_width, (srcY + 1) * char_height))

                    # Append character to our font png
                    output_image.paste(char_crop,
                                       (x * char_width, y * char_height,
                                        (x + 1) * char_width,
                                        (y + 1) * char_height))

                    # Write index
                    file.write("{},\n".format(hex(char_to_int(ch))))

                    x += 1
                    if (x >= chars_per_row):
                        x = 0
                        y += 1
            if fail:
                logger.error(
                    "Remove / Replace the unsupported characters in PO files, rerun \"new_translations.sh\" and regenerate fonts again"
                )
                return 1

            image = remove_red_dots(np.array(output_image, dtype=np.uint8))
            output_image = Image.fromarray(image, "RGB")
            output_image.save(dst_png_path.resolve())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='count', default=0)
    parser.add_argument('src_png', metavar='src_png', type=Path)
    parser.add_argument('src_png_jap', metavar='src_png_jap', type=Path)
    parser.add_argument('charset_option', metavar='charset_option', type=str)
    parser.add_argument('required_chars_path',
                        metavar='required_chars_path',
                        type=Path)
    parser.add_argument('char_width', metavar='char_width', type=int)
    parser.add_argument('char_height', metavar='char_height', type=int)
    parser.add_argument('dst_png', metavar='dst_png', type=Path)
    parser.add_argument('ipp_path', metavar='ipp_path', type=Path)

    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    retval = cmd_create_font_png(args.charset_option, args.required_chars_path,
                                 args.src_png, args.src_png_jap,
                                 args.char_width, args.char_height,
                                 args.ipp_path, args.dst_png)
    sys.exit(retval if retval is not None else 0)


if __name__ == '__main__':
    main()
