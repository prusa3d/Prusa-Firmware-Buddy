import argparse
import json
from enum import Enum
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, Optional, List
from pathlib import Path
import zlib

basic_types = {
    "int8_t", "int16_t", "int32_t", "uint8_t", "uint16_t", "uint32_t", "bool",
    "float", "char"
}


class DataType(Enum):
    i8 = "int8_t"
    i16 = "int16_t"
    i32 = "int32_t"
    u8 = "uint8_t"
    u16 = "uint16_t"
    u32 = "uint32_t"
    flt = "float"
    bool = "bool"
    char = "char"


def add_ifdef(ifdef: str, definition: str) -> str:
    res = f"#if {ifdef}\n "

    res = res + definition

    res = res + f"\n#endif"
    return res


@dataclass
class Item(ABC):
    name: str
    ifdef: Any

    def generate_item(self):
        pass

    def get_item_declaration(self) -> str:
        res = f"MemConfigItem<{self.get_type_name()}> {self.get_name()} {{ \"{self.name}\" , {str(self.default)} }};"

        if self.ifdef is not None:
            res = add_ifdef(self.ifdef, res)

        return res

    @abstractmethod
    def get_additional_definitions(self) -> Optional[str]:
        pass

    @abstractmethod
    def get_type_name(self) -> str:
        pass

    def get_name(self) -> str:
        return self.name

    @property
    @abstractmethod
    def default(self) -> str:
        pass


class BasicItem(Item):
    data_type: DataType
    def_val: Any

    def __init__(self, name: str, data_type: DataType, default: Any,
                 ifdef: Any):
        self.name = name
        self.data_type = data_type
        self.def_val = default
        self.ifdef = ifdef

    def get_additional_definitions(self) -> Optional[str]:
        return None

    def get_type_name(self):
        return self.data_type.value

    @property
    def default(self) -> str:
        if type(self.def_val) == bool:
            if self.def_val:
                return "true"
            else:
                return "false"
        else:
            return str(self.def_val)


@dataclass
class ArrayItem(Item):
    size: int
    item: Item

    def get_additional_definitions(self) -> Optional[str]:
        return None

    def get_type_name(self):
        return f"std::array<{self.item.get_type_name()},{self.size}>"

    @property
    def default(self) -> str:
        return self.item.default


@dataclass
class StructItem(Item):
    items: dict
    type_name: str

    def __get_members(self) -> str:
        members = ""
        for name, item in self.items.items():
            members = members + f"{item.get_type_name()} {name};\n"
        return members

    def __get_packer_function(self) -> str:
        packer = \
            f"""template <class Packer>
void pack (Packer &pack) {{
pack("""
        for name in self.items.keys():
            packer = packer + f"{name}, "
        return f"{str(packer[0:-2])});}}"

    def get_type_name(self):
        return self.type_name

    def get_additional_definitions(self) -> Optional[str]:
        return \
            f"""struct {self.get_type_name()} {{
{self.__get_members()}
{self.__get_packer_function()}
}};"""

    @property
    def default(self) -> str:
        bound = "{ "
        for item_name, item in self.items.items():
            bound = bound + f"{item.default}, "
        return str(bound[0:-2]) + " }"


class ItemParser:
    bounds: dict
    items: dict
    additional: list

    def __init__(self):
        self.bounds = dict()
        self.items = dict()
        self.additional = []

    def __create_basic_item(self,
                            name: str,
                            data_type: str,
                            default: Any,
                            ifdef=None) -> Item:
        try:
            data_type = DataType[data_type]
        except:
            raise Exception(f"{name} has invalid data type")

        return BasicItem(name, data_type, default, ifdef)

    def __create_array_item(
            self,
            name: str,
            data_type: str,
            length: int,
            item: dict,
            ifdef=None,
    ) -> Item:
        member = self.__parse_type("item", item)

        return ArrayItem(name, ifdef, length, member)

    def __create_struct_item(self,
                             name: str,
                             data_type: str,
                             type_name: str,
                             members: dict,
                             ifdef=None) -> Item:
        items = dict()
        for member_name, data in members.items():
            items[member_name] = self.__parse_type(name, data)
        return StructItem(name, ifdef, items, type_name)

    def __parse_type(self, name: str, data: dict) -> Item:
        item_type = data["data_type"]
        if item_type == "array":
            return self.__create_array_item(name, **data)
        elif item_type == "struct":
            return self.__create_struct_item(name, **data)
        elif item_type == "string":
            def_val = f"\"{data['default']}\""
            return self.__create_array_item(name, "char", data["length"], {
                "data_type": "char",
                "default": def_val
            })
        else:
            return self.__create_basic_item(name, **data)

    def parse_type(self, name: str, data: dict):
        crc = zlib.crc32(name.encode())
        item = self.items.get(crc)
        if item is not None:
            if name == item:
                raise Exception(f"Duplicate name: {item}")
            raise Exception(f"crc of {name} conflicts with crc of {item}")
        item = self.__parse_type(name, data)
        self.items[crc] = item
        return item

    def get_switch_cases(self) -> List[str]:
        cases = []
        for crc, item in self.items.items():
            line = f"case {str(crc)}:\n store.{item.name}.init(EepromAccess::deserialize_data<decltype(store.{item.name}.data)>(data).data);\n break;"
            if item.ifdef is not None:
                cases.append(add_ifdef(item.ifdef, line))
            else:
                cases.append(line)
        return cases


def main():
    parser = argparse.ArgumentParser(
        "Parse path to configuration store definition and path to output")
    parser.add_argument(
        "Configuration",
        help="Path to json file containing configuration store definition",
        type=Path)
    parser.add_argument("Output",
                        help="Path to where to store the output file",
                        type=Path)
    parser.add_argument("OutputSwitch",
                        help="Path to where to store the output file",
                        type=Path)

    args = parser.parse_args()
    items = []
    additional_definitions = []
    item_parser = ItemParser()
    with open(args.Configuration, "r") as config:
        for key, value in json.load(config).items():
            item = item_parser.parse_type(key, value)
            items.append(item)

    with open(args.Output, "w") as file:
        file.write("#pragma once\n")
        file.write("#include \"configuration_store/mem_config_item.hpp\"\n")
        file.write("namespace configuration_store{\n")
        for item in items:
            additional = item.get_additional_definitions()
            if additional is not None:
                file.write(additional)
                file.write("\n")
        file.write("struct ConfigurationStoreStructure {\n")
        file.write("private:\n")

        # file.write("protected:\n")
        file.write("public:\n")
        file.write(f"static constexpr size_t NUM_OF_ITEMS = {len(items)};\n")
        file.write("// items\n")
        for item in items:
            file.write(item.get_item_declaration())
            file.write("\n")

        file.write("auto tuplify(){\n return std::tie(\n")
        if items[0].ifdef is not None:
            file.write(f"{add_ifdef(items[0].ifdef, items[0].name)}\n")
        else:
            file.write(f"{items[0].name}\n")
        for item in items[1:]:
            line = f",{item.name}"
            if item.ifdef is not None:
                file.write(f"{add_ifdef(item.ifdef, line)}\n")
            else:
                file.write(f"{line}\n")
        file.write("); };\n};\n}\n")
    with open(args.OutputSwitch, "w") as file:
        file.write("#include \"configuration_store/item_updater.hpp\"\n")
        file.write("#include \"configuration_store/eeprom_access.hpp\"\n")
        file.write("using namespace configuration_store;\n")
        file.write(
            "#include \"configuration_store/configuration_store.hpp\"\n")
        file.write(
            "void ItemUpdater::operator()(uint32_t crc,const std::vector<uint8_t> &data) {\n"
        )
        file.write("switch(crc){\n")
        for case in item_parser.get_switch_cases():
            file.write(f"{case}\n")
        file.write("}\n")
        file.write("}\n")


if __name__ == "__main__":
    exit(main())
