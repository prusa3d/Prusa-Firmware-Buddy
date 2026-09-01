import yaml
import os
import numpy
import uuid
import sys
import typing
import cbor2
import io
import dataclasses
import re


@dataclasses.dataclass
class EncodeConfig:
    # Encode CBOR canonically (order map entries)
    canonical: bool = True

    # Encode using indefinite containers
    indefinite_containers: bool = True


# Without this bit of magic, the cbor2 Python library encodes some floats (for example 0.3) as 8 B doubles - we don't want that, it wastes space
class CompactFloat:
    decimal_precision = 3
    required_precision = pow(10, -decimal_precision)

    value: float

    def __init__(self, num: float):
        num = float(num)

        if num.is_integer():
            self.value = int(num)

        elif abs(num - float(numpy.float16(num))) < CompactFloat.required_precision:
            self.value = float(numpy.float16(num))

        elif abs(num - float(numpy.float32(num))) < CompactFloat.required_precision:
            self.value = float(numpy.float32(num))

        else:
            self.value = num

    @staticmethod
    def decode(data):
        num = float(data)
        return int(num) if num.is_integer() else round(num, CompactFloat.decimal_precision)

    def encode_cbor(self, encoder: cbor2.CBOREncoder):
        # Always encode floats canonically
        # Noncanonically, floats would always be encoded in 8 B, which is a lot of wasted space
        cbor2.CBOREncoder(encoder.fp, canonical=True).encode(self.value)


# Represent a raw CBOR data that are to be encoded verbatim
class RawCBORData:
    data: bytes

    def __init__(self, data: bytes):
        self.data = data

    def encode_cbor(self, encoder: cbor2.CBOREncoder):
        encoder.fp.write(self.data)


class Field:
    key: int
    name: str
    required: bool
    type_name: str

    def __init__(self, config, config_dir):
        self.type_name = config["type"]
        self.key = int(config["key"])
        self.name = str(config["name"])
        self.required = config.get("required", False)


class BoolField(Field):
    def decode(self, data):
        return bool(data)

    def encode(self, data):
        return bool(data)


class IntField(Field):
    def decode(self, data):
        return int(data)

    def encode(self, data):
        return int(data)


class NumberField(Field):
    def decode(self, data):
        return CompactFloat.decode(data)

    def encode(self, data):
        return CompactFloat(data)


class StringField(Field):
    max_len: int

    def __init__(self, config, config_dir):
        super().__init__(config, config_dir)
        self.max_len = config["max_length"]

    def decode(self, data):
        return str(data)

    def encode(self, data):
        result = str(data)
        assert len(result) <= self.max_len
        return result


class EnumFieldBase(Field):
    items_by_key: dict[str, int]
    items_by_name: dict[int, str]
    items_yaml: list[dict]

    def __init__(self, config, config_dir):
        super().__init__(config, config_dir)

        self.items_by_key = dict()
        self.items_by_name = dict()

        self.items_yaml = yaml.safe_load(open(os.path.join(config_dir, config["items_file"]), "r", encoding="utf-8"))
        for item in self.items_yaml:
            if item.get("deprecated", False):
                continue

            key = int(item[config.get("index_field", "key")])
            name = str(item[config.get("name_field", "name")])

            assert key not in self.items_by_key, f"Key '{key}' already exists"
            assert name not in self.items_by_name, f"Item '{name}' already exists"

            self.items_by_key[key] = name
            self.items_by_name[name] = key

    def decode(self, data):
        if not isinstance(data, int):
            raise ValueError("Enum item not integer")

        return self.items_by_key.get(data, data)

    def encode(self, data):
        if isinstance(data, str):
            return self.items_by_name[data]

        elif isinstance(data, int):
            # Pass unkown items verbatim
            return data

        else:
            raise ValueError("Enum values must be either")


class EnumField(EnumFieldBase):
    def __init__(self, config, config_dir):
        super().__init__(config, config_dir)


class EnumArrayField(EnumFieldBase):
    max_len: int

    def __init__(self, config, config_dir):
        super().__init__(config, config_dir)
        self.max_len = config["max_length"]

    def decode(self, data):
        assert type(data) is list

        return [EnumFieldBase.decode(self, item) for item in data]

    def encode(self, data):
        assert type(data) is list

        result = [EnumFieldBase.encode(self, item) for item in data]

        assert len(result) <= self.max_len
        return result


class ColorRGBAField(Field):
    def decode(self, data):
        assert isinstance(data, bytes)
        return f"#{data.hex()}"

    def encode(self, data):
        assert isinstance(data, str)
        m = re.match(r"^#([0-9a-f]{6}([0-9a-f]{2})?)$", data)
        assert m
        return bytes.fromhex(m.group(1))


class LABCborData:
    data: list

    def __init__(self, data: list):
        self.data = data

    def encode_cbor(self, encoder: cbor2.CBOREncoder):
        # Encode with definite container, it's smaller and the record is fixed size
        cbor2.CBOREncoder(encoder.fp, canonical=True, indefinite_containers=False, default=encoder.default).encode(self.data)


class ColorLABField(Field):
    def decode(self, data):
        assert type(data) is list
        assert len(data) == 3
        return [CompactFloat.decode(x) for x in data]

    def encode(self, data):
        assert type(data) is list
        assert len(data) == 3
        return LABCborData([CompactFloat(x) for x in data])


class UUIDField(Field):
    def decode(self, data):
        return str(uuid.UUID(bytes=data))

    def encode(self, data):
        return uuid.UUID(data).bytes


field_types = {
    "bool": BoolField,
    "int": IntField,
    "number": NumberField,
    "string": StringField,
    "enum": EnumField,
    "enum_array": EnumArrayField,
    "timestamp": IntField,
    "color_rgba": ColorRGBAField,
    "color_lab": ColorLABField,
    "uuid": UUIDField,
}


class Fields:
    fields_by_key: dict[int, Field]
    fields_by_name: dict[str, Field]

    def __init__(self):
        self.fields_by_key = dict()
        self.fields_by_name = dict()
        self.required_fields = list()

    def init_from_yaml(self, yaml, config_dir):
        for row in yaml:
            if row.get("deprecated", False):
                continue

            field_type_str = row.get("type")
            assert field_type_str, f"Field type not specified '{row}'"

            field_type = field_types.get(field_type_str)
            assert field_type, f"Unknown field type '{field_type_str}'"
            field = field_type(row, config_dir)

            assert field.key not in self.fields_by_key, f"Field {field.name} duplicit key {field.key}"
            assert field.name not in self.fields_by_name

            self.fields_by_key[field.key] = field
            self.fields_by_name[field.name] = field

    def from_file(file: str):
        r = Fields()
        r.init_from_yaml(yaml.safe_load(open(file, "r", encoding="utf-8")), os.path.dirname(file))

        return r

    # Decodes the fields and values from the CBOR binary data
    # If out_unknown_fields is provided, unknown fields are written into it instead of asserting
    def decode(self, binary_data: typing.IO[bytes], out_unknown_fields: dict[str, str] = None):
        data = cbor2.load(binary_data)
        result = dict()
        for key, value in data.items():
            field = self.fields_by_key.get(key)

            if field is None and out_unknown_fields is not None:
                # TODO: These would ideally be passed verbatim, avoiding the deserialize-serialize loop
                out_unknown_fields[cbor2.dumps(key).hex()] = cbor2.dumps(value).hex()
                continue

            assert field, f"Unknown CBOR key '{key}'"

            try:
                result[field.name] = field.decode(value)
            except Exception as e:
                e.add_note(f"Field {key} {field.name}")
                raise

        return result

    # Encodes keys and field values to a cbor-ready dictionary
    def encode(self, data: dict[str, any], config: EncodeConfig = EncodeConfig()) -> bytes:
        return self.update(update_fields=data, config=config)

    def update(
        self,
        original_data: typing.IO[bytes] = None,
        update_fields: dict[str, any] = {},
        update_unknown_fields: dict[str, str] = {},
        remove_fields: list[str] = [],
        config: EncodeConfig = EncodeConfig(),
    ) -> bytes:
        if original_data:
            result = cbor2.load(original_data)
        else:
            result = dict()

        for field_name in remove_fields:
            field = self.fields_by_name.get(field_name)
            assert field, f"Unknown field '{field_name}'"

            del result[field.key]

        for field_name, value in update_fields.items():
            field = self.fields_by_name.get(field_name)
            assert field, f"Unknown field '{field_name}'"

            try:
                result[field.key] = field.encode(value)
            except Exception as e:
                e.add_note(f"Field {field.key} {field.name}")
                raise

        # Enforce use of CompactFloat, the "default" float encoding is not optimal when canonical == False
        for field_name, value in result.copy().items():
            if isinstance(value, float):
                result[field_name] = CompactFloat(value)

        # Unknown fields pass verbatim
        for key, value in update_unknown_fields.items():
            result[RawCBORData(bytes.fromhex(key))] = RawCBORData(bytes.fromhex(value))

        data_io = io.BytesIO()
        encoder = cbor2.CBOREncoder(
            data_io,
            canonical=config.canonical,
            indefinite_containers=config.indefinite_containers,
            default=lambda enc, data: data.encode_cbor(enc),
        )

        encoder.encode(result)
        return data_io.getvalue()
