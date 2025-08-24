#!/usr/bin/env python3

import sys
import itertools


def get_lines(path):
    with open(path, 'r') as file:
        return file.readlines()


def main():
    try:
        expected_lines = sorted(
            itertools.chain(get_lines('utils/holly/build-requirements.txt'),
                            get_lines('utils/holly/heavy-requirements.txt')))
        actual_lines = get_lines('requirements.txt')

        if actual_lines == expected_lines:
            return 0
        else:
            with open('requirements.txt', 'w') as file:
                file.writelines(expected_lines)
            return 1

    except Exception as ex:
        print(ex)
        return 1


if __name__ == '__main__':
    sys.exit(main())
