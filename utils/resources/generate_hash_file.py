import sys
import os
import argparse

from pathlib import Path


def generate_header_file(*, input_hash, namespace, variable_name, output_path):
    content = f"""
    #pragma once
    #include <stdint.h>
    #include "resources/revision.hpp"

    namespace {namespace} {{
        static const Revision {variable_name} = {{ {{ { ", ".join(str(b) for b in input_hash) } }} }};
    }};
    """
    os.makedirs(output_path.parent, exist_ok=True)
    with open(output_path, 'w') as f:
        f.write(content)


def main(args):
    input_path: Path = getattr(args, 'hash-file')
    input_hash = input_path.read_bytes()
    output_path = getattr(args, 'output-file')
    generate_header_file(input_hash=input_hash,
                         namespace=args.header_namespace_name,
                         variable_name=args.header_variable_name,
                         output_path=output_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('hash-file', type=Path)
    parser.add_argument('output-file', type=Path)
    parser.add_argument('--header-namespace-name', required=False)
    parser.add_argument('--header-variable-name', required=False)
    args = parser.parse_args(sys.argv[1:])
    sys.exit(main(args) or 0)
