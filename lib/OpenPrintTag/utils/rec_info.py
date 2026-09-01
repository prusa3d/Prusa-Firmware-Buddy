import argparse
import json
import sys
import urllib.parse
from pathlib import Path

import jsonschema
import jsonschema.validators
import referencing
import yaml
from common import default_config_file
from opt_check import opt_check
from record import Record

parser = argparse.ArgumentParser(prog="rec_info", description="Reads a record from the STDIN and prints various information about it in the YAML format")
parser.add_argument("-c", "--config-file", type=str, default=default_config_file, help="Record configuration YAML file")
parser.add_argument("-r", "--show-region-info", action=argparse.BooleanOptionalAction, default=False, help="Print information about regions")
parser.add_argument("--show-field-info", action=argparse.BooleanOptionalAction, default=False, help="Show per-field used size (including key). Please note that due to cbor2 constraints, the values represent the field size this utility would re-encode into, not the actual data encoded size.")
parser.add_argument("-u", "--show-root-info", action=argparse.BooleanOptionalAction, default=False, help="Print general info about the NFC tag")
parser.add_argument("-d", "--show-data", action=argparse.BooleanOptionalAction, default=False, help="Parse and print region data")
parser.add_argument("-b", "--show-raw-data", action=argparse.BooleanOptionalAction, default=False, help="Print raw region data (HEX)")
parser.add_argument("-m", "--show-meta", action=argparse.BooleanOptionalAction, default=False, help="By default, --show-data hides the meta region. Enabling this option will print it, too.")
parser.add_argument("-i", "--show-uri", action=argparse.BooleanOptionalAction, default=False, help="If a URI NDEF record is present, report it as well.")
parser.add_argument("-a", "--show-all", action=argparse.BooleanOptionalAction, default=False, help="Apply (almost) all --show options")
parser.add_argument("-v", "--validate", action=argparse.BooleanOptionalAction, default=False, help="Check that the data are valid")
parser.add_argument("-f", "--extra-required-fields", type=str, default=None, help="Check that all fields from the specified YAML file are present in the record")
parser.add_argument("--unhex", action=argparse.BooleanOptionalAction, default=False, help="Interpret the stdin as a hex string instead of raw bytes")
parser.add_argument("--opt-check", action=argparse.BooleanOptionalAction, default=False, help="Perform semantic checks (using opt_check.py)")
parser.add_argument("--tag-uid", type=str, default=None, help="UID of the tag for deriving the instance_uuid with --opt-check. Hex format, NFC-V UIDs should start with 'E0'")

args = parser.parse_args()

if args.show_all:
    args.show_region_info = True
    args.show_field_info = True
    args.show_root_info = True
    args.show_data = True
    args.show_raw_data = True
    args.show_meta = True
    args.show_uri = True

data = sys.stdin.buffer.read()

if args.unhex:
    data = data.decode()
    data = data.replace("0x", "").replace(" ", "")
    data = bytearray.fromhex(data)
else:
    data = bytearray(data)

record = Record(args.config_file, memoryview(data))
output = {}
return_fail = False

if args.show_region_info or args.show_root_info:
    regions_info = dict()
    payload_used_size = 0

    for name, region in record.regions.items():
        region_info = region.info_dict()
        payload_used_size += region.used_size()

        if args.show_field_info:
            field_sizes = dict()

            base_len = len(region.fields.update())

            for key, value in region.read(out_unknown_fields=dict()).items():
                field_sizes[key] = len(region.fields.update(update_fields={key: value})) - base_len

            region_info["field_used_size"] = field_sizes

        regions_info[name] = region_info

    if args.show_region_info or args.show_field_info:
        output["regions"] = regions_info

    if args.show_root_info:
        overhead = len(record.data) - len(record.payload)
        output["root"] = {
            **record.root_info,
            "data_size": len(record.data),
            "payload_size": len(record.payload),
            "overhead": overhead,
            "payload_used_size": payload_used_size,
            "total_used_size": payload_used_size + overhead,
        }

if args.show_data:
    data = {}
    unknown_fields = {}

    for name, region in record.regions.items():
        if name == "meta" and not args.show_meta:
            continue

        region_unknown_fields = dict()
        data[name] = region.read(out_unknown_fields=region_unknown_fields)

        if len(region_unknown_fields) > 0:
            unknown_fields[name] = region_unknown_fields

    output["data"] = data

    if len(unknown_fields):
        output["unknown_fields"] = unknown_fields

if args.show_raw_data:
    data = {}

    for name, region in record.regions.items():
        if args.show_meta or name != "meta":
            data[name] = region.memory.hex()

    output["raw_data"] = data

if args.show_uri:
    output["uri"] = record.uri

if args.validate or args.opt_check:
    validate_result = record.validate()
    output["validate"] = validate_result

    if len(validate_result["errors"]) > 0:
        return_fail = True

if args.extra_required_fields:
    with open(args.extra_required_fields, "r", encoding="utf-8") as f:
        req_fields = yaml.safe_load(f)

    for region_name, region_req_fields in req_fields.items():
        region = record.regions.get(region_name)
        assert region, f"Missing region {region_name}"

        region_data = region.read()

        for req_field_name in region_req_fields:
            assert req_field_name in region_data, f"Missing field '{req_field_name}' in region '{region_name}'"

if args.opt_check:
    if args.tag_uid:
        tag_uid = bytes.fromhex(args.tag_uid)
    else:
        tag_uid = None

    opt_check_result = opt_check(record, tag_uid)
    output["opt_check"] = opt_check_result

    if len(opt_check_result["errors"]) > 0:
        return_fail = True


# Check that the output of this utility is up to the spec
def validate_output_with_json_schema():
    def file_retrieve(uri):
        path = Path(__file__).parent / "schema" / urllib.parse.urlparse(uri).path
        result = json.loads(path.read_text(encoding="utf-8"))
        return referencing.Resource.from_contents(result)

    registry = referencing.Registry(retrieve=file_retrieve)
    entry = "opt_json.schema.json"

    schema = registry.get_or_retrieve(entry).value.contents
    validator = jsonschema.validators.validator_for(schema)(schema, registry=registry)
    validator.validate(output)


validate_output_with_json_schema()


def yaml_hex_bytes_representer(dumper: yaml.SafeDumper, data: bytes):
    return dumper.represent_str("0x" + data.hex())


class InfoDumper(yaml.SafeDumper):
    pass


InfoDumper.add_representer(bytes, yaml_hex_bytes_representer)
yaml.dump(output, stream=sys.stdout, Dumper=InfoDumper, sort_keys=False)

sys.exit(1 if return_fail else 0)
