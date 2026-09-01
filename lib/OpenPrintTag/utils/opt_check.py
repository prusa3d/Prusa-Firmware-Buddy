import argparse
import sys
import yaml
import inspect
import itertools
import uuid
import re

from record import Record
from common import default_config_file


def opt_check(rec: Record, tag_uid: bytes = None):
    warnings = list()
    errors = list()
    notes = list()
    uuids = dict()

    # Pass empty dict to out_unknown_fields so that the script does not crash if the tag has unkown fields
    # Unknown fields should by reported by rec_info --validate
    main_data = rec.main_region.read(out_unknown_fields={})

    # Aux region checks
    if rec.aux_region is None:
        warnings.append("Aux region not present")
    else:
        if len(rec.aux_region.memory) < 16:
            warnings.append("Aux region is smaller than 16 bytes")

    # Check tag transitivities
    data_tags = main_data.get("tags", [])
    for tag_data in rec.main_region.fields.fields_by_name["tags"].items_yaml:
        if tag_data.get("deprecated", False):
            continue

        tag_name = tag_data["name"]
        if tag_name not in data_tags:
            # We don't have this tag, no problem
            continue

        for implication in tag_data.get("implies", []):
            if implication not in data_tags:
                # Not an error, just a warning - if the data is older, the implied tag might not have existed
                warnings.append(f"Tag '{tag_name}' present but implied tag '{implication}' not")

        for hint in tag_data.get("hints", []):
            if hint not in data_tags:
                notes.append(f"Consider adding tag '{hint}' (hinted by '{tag_name}')")

    # Sanity-check some fields
    def check_relation(fields: list[str], func, error=None):
        for field_a, field_b in itertools.combinations(fields, 2):
            if (field_a not in main_data) or (field_b not in main_data):
                # Fields not present - cannot check
                continue

            val_a = main_data[field_a]
            val_b = main_data[field_b]

            if func(val_a, val_b):
                # Ok
                continue

            if error is None:
                error = inspect.getsource(func)
                error = re.sub(r"^.*lambda[^:]*:([^,)]+).*$", "\\1", error)
                error = error.strip()

            errors.append(f"Fields {field_a} ({val_a}), {field_b} ({val_b}): {error}")

    check_relation(["nominal_netto_full_weight", "actual_netto_full_weight"], lambda a, b: a <= b)
    check_relation(["nominal_full_length", "actual_full_length"], lambda a, b: a <= b)

    check_relation(["preheat_temperature", "min_print_temperature", "max_print_temperature"], lambda a, b: a <= b)
    check_relation(["min_bed_temperature", "max_bed_temperature"], lambda a, b: a <= b)
    check_relation(["min_chamber_temperature", "chamber_temperature", "max_chamber_temperature"], lambda a, b: a <= b)

    check_relation(["container_hole_diameter", "container_inner_diameter", "container_outer_diameter"], lambda a, b: a <= b)

    # Check and deduce UUIDs
    def generate_uuid(namespace, *args):
        return uuid.uuid5(uuid.UUID(namespace), b"".join(args))

    def deduce_uuid(field, generated_uuid, report_deduce_fail: bool = True):
        if explicit_uuid := main_data.get(field):
            result = uuid.UUID(explicit_uuid)

            if result == generated_uuid:
                warnings.append(f"{field} is identical to the auto-generated version, and thus can be omitted to save space")

        elif generated_uuid:
            result = generated_uuid

        else:
            if report_deduce_fail:
                warnings.append(f"Failed to deduce {field}")

            result = None

        if generated_uuid and result != generated_uuid:
            notes.append(f"{field} ({result}) differes from auto-generated {generate_uuid}")

        uuids[field] = str(result) if result else None

    if brand_name := main_data.get("brand_name"):
        brand_generated_uuid = generate_uuid("5269dfb7-1559-440a-85be-aba5f3eff2d2", brand_name.encode("utf-8"))
    else:
        brand_generated_uuid = None

    deduce_uuid("brand_uuid", brand_generated_uuid)

    if (brand_uuid := uuids["brand_uuid"]) and (material_name := main_data.get("material_name")):
        material_generated_uuid = generate_uuid("616fc86d-7d99-4953-96c7-46d2836b9be9", uuid.UUID(brand_uuid).bytes, material_name.encode("utf-8"))
    else:
        material_generated_uuid = None

    deduce_uuid("material_uuid", material_generated_uuid)

    if (brand_uuid := uuids["brand_uuid"]) and (gtin := main_data.get("gtin")):
        package_generated_uuid = generate_uuid("6f7d485e-db8d-4979-904e-a231cd6602b2", uuid.UUID(brand_uuid).bytes, str(gtin).encode("utf-8"))
    else:
        package_generated_uuid = None

    deduce_uuid("package_uuid", package_generated_uuid)

    if tag_uid and tag_uid[0] != 0xE0:
        warnings.append(f"Tag UID {tag_uid.hex()} doesn't start with 0xE0")

    if (brand_uuid := uuids["brand_uuid"]) and tag_uid:
        assert tag_uid[0] == 0xE0, "Make sure tag_uid is in the correct byte order"
        instance_generated_uuid = generate_uuid("31062f81-b5bd-4f86-a5f8-46367e841508", tag_uid)
    else:
        instance_generated_uuid = None

    deduce_uuid("instance_uuid", instance_generated_uuid, report_deduce_fail=False)

    return {
        "warnings": warnings,
        "errors": errors,
        "notes": notes,
        "uuids": uuids,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="opt_check", description="Reads a record from the STDIN and performs validations and checks of the OpenPrintTag data. Results are returned to STDOUT in the YAML format.")
    parser.add_argument("-c", "--config-file", type=str, default=default_config_file, help="Record configuration YAML file")
    parser.add_argument("--uid", type=str, default=None, help="UID of the tag, as binary HEX string (starting with E0)")
    parser.add_argument("--unhex", action=argparse.BooleanOptionalAction, default=False, help="Interpret the stdin as a hex string instead of raw bytes")

    args = parser.parse_args()

    data = sys.stdin.buffer.read()

    if args.unhex:
        data = data.decode()
        data = data.replace("0x", "").replace(" ", "")
        data = bytearray.fromhex(data)
    else:
        data = bytearray(data)

    record = Record(args.config_file, memoryview(data))
    check_output = opt_check(record, args.uid)
    yaml.dump(check_output, stream=sys.stdout)

    if len(check_output["errors"]) > 0:
        sys.exit(1)
