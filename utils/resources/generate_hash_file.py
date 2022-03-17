import hashlib
import sys
import argparse


def generate_header_file(*, input_hash, namespace, variable_name, output_path):
    content = f"""
    #pragma once
    #include <stdint.h>
    #include "resources/revision.hpp"

    namespace {namespace} {{
        static const Revision {variable_name} = {{ {{ { ", ".join(str(b) for b in input_hash.digest()) } }} }};
    }};
    """
    with open(output_path, 'w') as f:
        f.write(content)


def generate_binary_file(*, input_hash, output_path):
    with open(output_path, 'wb') as f:
        f.write(input_hash.digest())


def main(args):
    input_path = getattr(args, 'input-file')
    output_path = getattr(args, 'output-file')
    with open(input_path, 'rb') as f:
        input_hash = hashlib.sha256(f.read())
    if args.format == 'binary':
        generate_binary_file(input_hash=input_hash, output_path=output_path)
    elif args.format == 'header':
        generate_header_file(input_hash=input_hash,
                             namespace=args.header_namespace_name,
                             variable_name=args.header_variable_name,
                             output_path=output_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--format',
                        choices=['binary', 'header'],
                        required=True)
    parser.add_argument('input-file')
    parser.add_argument('output-file')
    parser.add_argument('--header-namespace-name', required=False)
    parser.add_argument('--header-variable-name', required=False)
    args = parser.parse_args(sys.argv[1:])
    sys.exit(main(args) or 0)
