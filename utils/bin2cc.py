from pathlib import Path
import sys
import argparse
import logging

logger = logging.getLogger('bin2cc.py')


def bin2cc(src_filename: Path, dst_filename: Path, var_name: str):
    if not src_filename.exists():
        logger.error('no {} found', src_filename.name)
        return 1
    with open(src_filename.resolve(),
              "rb") as src_file, open(str(dst_filename), "w") as dst_file:
        dst_file.write("const uint8_t {}[] = ".format(str(var_name)) + "{\n")
        byte = src_file.read(1)
        while byte:
            dst_file.write("\t0x{},\n".format(byte.hex()))
            byte = src_file.read(1)
        dst_file.write("};\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='count', default=0)

    parser.add_argument('src_filename',
                        metavar='src_filename',
                        type=Path,
                        help="path to png to write to .c file")
    parser.add_argument('dst_filename',
                        metavar='dst_filename',
                        type=Path,
                        help="path where will be the .c file written")
    parser.add_argument('var_name',
                        metavar='var_name',
                        type=str,
                        help="name of the variable in .c file")

    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    retval = bin2cc(args.src_filename, args.dst_filename, args.var_name)
    sys.exit(retval if retval is not None else 0)


if __name__ == '__main__':
    main()
