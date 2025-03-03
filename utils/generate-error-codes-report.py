#!/usr/bin/env python3

import yaml
from collections import defaultdict


def process_yaml_dict(root, result):
    for ec in root['Errors']:
        approved = ec['approved']
        code = ec['code']
        printers = ec.get('printers', [])
        if not approved:
            result[code].extend(printers)


def process_yaml_file(filename, result):
    with open(filename, 'r') as f:
        process_yaml_dict(yaml.full_load(f), result)


def report_result(result):
    if result:
        print('Some error codes are not approved, please fix them 🙏')
        for ec, printers in result.items():
            printers_str = ', '.join(
                sorted(printers)).lower() if printers else 'all'
            print(f'* `{ec}` ({printers_str})')
    else:
        print('All error codes are approved 💪')


def main():
    result = defaultdict(list)
    process_yaml_file('lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml',
                      result)
    report_result(result)


if __name__ == '__main__':
    main()
