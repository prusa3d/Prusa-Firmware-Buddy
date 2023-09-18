from pathlib import Path
import sys
import argparse
import logging
import math

logger = logging.getLogger('bin2cc.py')


def bin2cc(src_filename: Path, dst_filename: Path, var_name: str, w: int,
           h: int):
    if not src_filename.exists():
        logger.error('no {} found', src_filename.name)
        return 1
    with open(src_filename.resolve(),
              "rb") as src_file, open(str(dst_filename), "w") as dst_file:
        dst_file.write("const uint8_t font_{}_{}x{}_data[] = ".format(
            str(var_name), str(w), str(h)) + "{\n")
        byte = src_file.read(1)
        while byte:
            dst_file.write("    0x{},\n".format(byte.hex()))
            byte = src_file.read(1)
        dst_file.write("};\n")
        dst_file.write(
            f"const font_t font_{var_name}_{w}x{h} = {{ {w}, {h}, {math.ceil(w/2)}, 0, (uint16_t *)font_{var_name}_{w}x{h}_data, 32, 255 }};\n"
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='count', default=0)

    parser.add_argument('src_filename',
                        metavar='src_filename',
                        type=Path,
                        help="path to .bin to write to .hpp file")
    parser.add_argument('dst_filename',
                        metavar='dst_filename',
                        type=Path,
                        help="path where will be the .hpp file written")
    parser.add_argument('type',
                        metavar='type',
                        type=str,
                        help="name of the font type; eg regular/bold")
    parser.add_argument('width',
                        metavar='width',
                        type=int,
                        help="width of given font")
    parser.add_argument('height',
                        metavar='height',
                        type=int,
                        help="height of given font")

    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    retval = bin2cc(args.src_filename, args.dst_filename, args.type,
                    args.width, args.height)
    sys.exit(retval if retval is not None else 0)


if __name__ == '__main__':
    main()
