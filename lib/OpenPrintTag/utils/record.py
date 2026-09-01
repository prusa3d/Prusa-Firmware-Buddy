import io
import os
import types
import typing

import cbor2
import ndef
import yaml
from fields import EncodeConfig, Fields


class Region:
    memory: memoryview
    offset: int  # Offset of the region relative to payload start
    fields: Fields
    record: typing.Any
    is_corrupt: bool = False

    def __init__(self, record, offset: int, memory: memoryview, fields: Fields):
        assert type(memory) is memoryview
        assert len(memory) <= 512, "Specification prohibits memory regions larger than 512 bytes"

        self.record = record
        self.offset = offset
        self.memory = memory
        self.fields = fields

        try:
            cbor2.load(io.BytesIO(self.memory))
        except cbor2.CBORError:
            self.is_corrupt = True

        if len(self.memory) == 0:
            self.is_corrupt = True

    def info_dict(self):
        result = {
            "payload_offset": self.offset,
            "absolute_offset": self.offset + self.record.payload_offset,
            "size": len(self.memory),
            "used_size": self.used_size(),
        }

        if self.is_corrupt:
            result["is_corrupt"] = True

        return result

    def used_size(self):
        if self.is_corrupt:
            return 0

        data_io = io.BytesIO(self.memory)
        cbor2.load(data_io)
        return data_io.tell()

    def read(self, out_unknown_fields: dict[any, any] = None) -> dict[str, any]:
        if self.is_corrupt:
            return {}

        return self.fields.decode(io.BytesIO(self.memory), out_unknown_fields=out_unknown_fields)

    def write(self, data: dict[str, any]):
        return self.update(data, clear=True)

    def update(self, update_fields: dict[str, any], update_unknown_fields: dict[str, str] = {}, remove_fields: list[str] = [], clear: bool = False):
        if len(update_fields) == 0 and len(remove_fields) == 0 and not clear:
            # Nothing to do
            return

        encoded = self.fields.update(
            original_data=io.BytesIO(self.memory) if not clear else None,
            update_fields=update_fields,
            remove_fields=remove_fields,
            update_unknown_fields=update_unknown_fields,
            config=self.record.encode_config,
        )
        encoded_len = len(encoded)

        assert encoded_len <= len(self.memory), f"Data of size {encoded_len} does not fit into region of size {len(self.memory)}"

        # Write zeroes to the whole region
        self.memory[:] = bytearray(len(self.memory))
        self.memory[0:encoded_len] = encoded
        return encoded_len


class Record:
    data: memoryview
    payload: memoryview
    payload_offset: int  # Offset of the payload relative to the NDEF message start
    config: types.SimpleNamespace
    config_dir: str
    uri: str = None

    meta_region: Region = None
    main_region: Region = None
    aux_region: Region = None

    regions: dict[str, Region] = None

    # Debug information about the record - to be shown in the CLI if the user demands it
    root_info: dict[str, typing.Any]

    encode_config: EncodeConfig

    def __init__(self, config_file: str, data: memoryview):
        assert type(data) is memoryview

        self.data = data
        self.encode_config = EncodeConfig()
        self.root_info = dict()

        self.config_dir = os.path.dirname(config_file)
        with open(config_file, "r", encoding="utf-8") as f:
            self.config = types.SimpleNamespace(**yaml.safe_load(f))

        # Decode the root and find payload
        match self.config.root:
            case "none":
                self.payload = data
                self.payload_offset = 0

            case "nfcv":
                data_io = io.BytesIO(data)
                cc = data_io.read(4)

                # TODO: Support 8-byte CC (with a different magic)
                assert cc[0] == 0xE1, "Capability container magic number does not match"

                self.root_info["cc_capacity"] = cc[2] * 8

                # Find the NDEF TLV
                while True:
                    base_tlv = data_io.read(2)
                    tag = base_tlv[0]

                    # Either gone out of range or hit a terminator TLV
                    if (tag is None) or (tag == 0xFE):
                        assert base_tlv is not None, "Did not found NDEF TLV"

                    tlv_len = base_tlv[1]

                    # 0xFF means that length takes two bytes
                    if tlv_len == 0xFF:
                        ext_len = data_io.read(2)
                        assert ext_len is not None
                        tlv_len = ext_len[0] * 256 | ext_len[1]

                    # 0x03 = NDEF TLV
                    if tag == 0x03:
                        # Found it

                        self.root_info["ndef_tlv_payload_start"] = data_io.tell()
                        self.root_info["ndef_tlv_payload_size"] = tlv_len
                        break
                    else:
                        # Skip the TLV block
                        data_io.seek(tlv_len, 1)

                for record in ndef.message_decoder(data_io):
                    if type(record) is ndef.UriRecord:
                        self.uri = record.uri

                    if record.type == self.config.mime_type:
                        # We have to create a sub memoryview so that when we update the region, the outer data updates as well
                        end = data_io.tell()
                        self.payload_offset = end - len(record.data)
                        self.payload = data[self.payload_offset : end]
                        assert self.payload == record.data
                        break

                else:
                    raise Exception(f"Did not find a record of type '{self.config.mime_type}'")

            case _:
                raise Exception(f"Unknown root type '{self.config.root}'")

        assert type(self.payload) is memoryview
        self._setup_regions()

    # Validates the region and reports possible errors
    def validate(self):
        warnings = list()
        errors = list()

        # Check we have all required & recommended fields
        for region_name, region in self.regions.items():
            unknown_fields = {}
            region_data = region.read(out_unknown_fields=unknown_fields)

            if len(unknown_fields) > 0:
                warnings.append(f"Region '{region_name}' contains unknown fields")

            for field in region.fields.fields_by_name.values():
                if field.name in region_data:
                    pass  # Has the field, no problem

                elif field.required == "recommended":
                    warnings.append(f"Missing recommended field '{field.name}'")

                elif field.required:
                    errors.append(f"Missing required field '{field.name}'")

        return {
            "warnings": warnings,
            "errors": errors,
        }

    def _setup_regions(self):
        if "meta_fields" not in self.config.__dict__:
            # If meta region is not present, we only have the main region which spans the entire payload
            self.main_region = Region(0, self.payload, Fields.from_file(os.path.join(self.config_dir, self.config.main_fields)))
            self.regions = {"main", self.main_region}
            return

        meta_io = io.BytesIO(self.payload)
        cbor2.load(meta_io)
        meta_section_size = meta_io.tell()
        metadata = Region(self, 0, self.payload[0:meta_section_size], Fields.from_file(os.path.join(self.config_dir, self.config.meta_fields))).read()

        main_region_offset = metadata.get("main_region_offset", meta_section_size)
        main_region_size = metadata.get("main_region_size")

        aux_region_offset = metadata.get("aux_region_offset")
        aux_region_size = metadata.get("aux_region_size")
        has_aux_region = aux_region_offset is not None
        assert (not has_aux_region) or (aux_region_size is None), "aux_region_size present without aux_region_offset"

        region_stops = list(filter(lambda x: x is not None, [main_region_offset, aux_region_offset, len(self.payload)]))
        region_stops.sort()

        def create_region(offset, size, fields):
            if size is None:
                size = list(filter(lambda a: a > offset, region_stops))[0] - offset

            result = Region(self, offset, self.payload[offset : offset + size], Fields.from_file(os.path.join(self.config_dir, fields)))

            if len(result.memory) != size:
                result.is_corrupt = True

            return result

        self.meta_region = create_region(0, None, self.config.meta_fields)
        self.main_region = create_region(main_region_offset, main_region_size, self.config.main_fields)
        self.regions = {"meta": self.meta_region, "main": self.main_region}

        if has_aux_region:
            self.aux_region = create_region(aux_region_offset, aux_region_size, self.config.aux_fields)
            self.regions["aux"] = self.aux_region
