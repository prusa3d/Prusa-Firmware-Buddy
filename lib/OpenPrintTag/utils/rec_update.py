import sys
import argparse
import yaml

from record import Record
from common import default_config_file

parser = argparse.ArgumentParser(prog="rec_update", description="Reads a record from STDIN and updates its fields according to the provided YAML file. Updated record is then printed to stdout.")
parser.add_argument("update_data", help="YAML file with instructions how to update the file")
parser.add_argument("-c", "--config-file", type=str, default=default_config_file, help="Record configuration YAML file")
parser.add_argument("--clear", action=argparse.BooleanOptionalAction, default=False, help="If set, the regions mentioned in the YAML file will be cleared rather than updated")
parser.add_argument("--indefinite-containers", action=argparse.BooleanOptionalAction, default=True, help="Encode CBOR containers as indefinite (using stop code instead of specifying length)")
parser.add_argument("--canonical", action=argparse.BooleanOptionalAction, default=True, help="Encode the CBOR maps canonically (order map keys)")

args = parser.parse_args()

record = Record(args.config_file, memoryview(bytearray(sys.stdin.buffer.read())))
record.encode_config.canonical = args.canonical
record.encode_config.indefinite_containers = args.indefinite_containers

update_data = yaml.safe_load(open(args.update_data, "r", encoding="utf-8"))
for region_name, region in record.regions.items():
    region.update(
        update_fields=update_data.get("data", dict()).get(region_name, dict()),
        remove_fields=update_data.get("remove", dict()).get(region_name, dict()),
        update_unknown_fields=update_data.get("unknown_fields", dict()).get(region_name, dict()),
        clear=args.clear,
    )

sys.stdout.buffer.write(record.data)
