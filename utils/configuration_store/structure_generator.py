import argparse
import json
from enum import Enum
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, Optional, List
from pathlib import Path
import zlib
import os


def add_ifdef(ifdef: str, definition: str) -> str:
    res = f"#if {ifdef}\n "

    res = res + definition

    res = res + f"\n#endif"
    return res


@dataclass
class Item(ABC):
    name: str
    ifdef: Any
    deprecated: False

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
    def get_additional_declarations(self) -> Optional[str]:
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

    @property
    def crc32(self):
        return zlib.crc32(self.name.encode())

    @abstractmethod
    def default_val_as_array(self, size: int):
        pass


class BasicItem(Item):
    data_type: str
    def_val: Any

    def __init__(self, name: str, data_type: str, default: Any, ifdef: Any,
                 deprecated: bool):
        self.name = name
        self.data_type = data_type
        self.def_val = default
        self.ifdef = ifdef
        self.deprecated = deprecated

    def get_additional_declarations(self) -> Optional[str]:
        return None

    def get_additional_definitions(self) -> Optional[str]:
        return None

    def get_type_name(self):
        return self.data_type

    @property
    def default(self) -> str:
        if type(self.def_val) == bool:
            if self.def_val:
                return "true"
            else:
                return "false"
        if type(self.def_val) == list:
            return f"{{ {','.join(str(val) for val in self.def_val)} }}"
        else:
            return str(self.def_val)

    def default_val_as_array(self, size: int) -> str:
        if type(self.def_val) == list:
            if len(self.def_val) != size:
                raise RuntimeError(
                    "Different length of default value and array")
            return f"{{ {','.join(str(val) for val in self.def_val)} }}"
        else:
            return f"{{ {','.join(str(self.def_val) for val in range(size))} }}"
        pass


@dataclass
class ArrayItem(Item):
    size: int
    item: Item

    def get_additional_definitions(self) -> Optional[str]:
        return self.item.get_additional_definitions()

    def get_additional_declarations(self) -> Optional[str]:
        return self.item.get_additional_declarations()

    def get_type_name(self):
        return f"std::array<{self.item.get_type_name()},{self.size}>"

    @property
    def default(self) -> str:
        return self.item.default_val_as_array(self.size)

    def default_val_as_array(self, size: int):
        return f"{{ {','.join(str(self.item.default) for val in range(size))} }}"


@dataclass
class StringItem(Item):
    size: int
    item: Item

    # TODO: check length of default value if it is array

    def get_additional_definitions(self) -> Optional[str]:
        return None

    def get_additional_declarations(self) -> Optional[str]:
        return None

    def get_type_name(self):
        return f"std::array<{self.item.get_type_name()},{self.size}>"

    @property
    def default(self) -> str:
        return self.item.default

    def default_val_as_array(self, size: int):
        return f"{{ {','.join(str(self.item.default) for val in range(size))} }}"


@dataclass
class StructItem(Item):
    items: dict
    type_name: str
    functions: list[str]

    def __get_members(self) -> str:
        members = ""
        for name, item in self.items.items():
            members = members + f"{item.get_type_name()} {name};\n"
        return members

    def __get_packer_function(self) -> str:
        return \
            f"""template <class Packer>
void pack (Packer &pack) {{
pack( {', '.join(self.items.keys())}); }}"""

    def __get_equality_operator(self):
        declaration_eq = f"bool operator==(const {self.type_name} & rhs) "
        conditions = " && ".join(
            [f"{name} == rhs.{name}" for name in self.items.keys()])
        body = f"{{\nreturn {conditions};\n }}\n"
        eq = declaration_eq + body
        neq = f"""bool operator!=(const {self.type_name} & rhs) {{
         return !(*this == rhs);
         }}\n"""
        return eq + neq

    def __get_accessor_function_declarations(self) -> list[str]:
        declarations = []
        for name, data in self.items.items():
            declarations.append(f"{data.get_type_name()} get_{name}();")
            declarations.append(f"void set_{name}({data.get_type_name()});")
        return declarations

    def __get_accessor_function_definitions(self):
        definitions = []
        for name, data in self.items.items():
            definitions.append(
                f"""{data.get_type_name()} {self.get_type_name()}::get_{name}(){{
            return {name};
            }}
            """)

            definitions.append(
                f"""void {self.get_type_name()}::set_{name}({data.get_type_name()} new_data){{
                {name} = new_data;
                config_store().{self.name}.set(*this);
            }}""")
        return definitions

    def default_val_as_array(self, size: int):
        return f"{{ {','.join(str(self.default) for val in range(size))} }}"

    def get_additional_definitions(self) -> Optional[str]:
        return "\n".join(self.__get_accessor_function_definitions())

    def get_type_name(self):
        return self.type_name

    def get_additional_declarations(self) -> Optional[str]:
        res = \
            f"""struct {self.get_type_name()} {{
{self.__get_members()}
{self.__get_packer_function()}
{self.__get_equality_operator()}
{os.linesep.join(self.__get_accessor_function_declarations())}
}};"""
        if self.functions is not None:
            res = res + f"\n{os.linesep.join(f'{func};' for func in self.functions)}"

        return res

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
        self.items = dict()
        self.additional = []

    def __create_basic_item(
            self,
            name: str,
            data_type: str,
            default: Any,
            ifdef=None,
            deprecated=False,
    ) -> Item:

        return BasicItem(name, data_type, default, ifdef, deprecated)

    def __create_string_item(
            self,
            name: str,
            data_type: str,
            length: int,
            item: dict,
            ifdef=None,
            deprecated=False,
    ):
        member = self.__parse_type("item", item)

        return StringItem(name, ifdef, deprecated, length, member)

    def __create_array_item(
            self,
            name: str,
            data_type: str,
            length: int,
            item: dict,
            ifdef=None,
            deprecated=False,
    ) -> Item:
        member = self.__parse_type("item", item)

        return ArrayItem(name, ifdef, deprecated, length, member)

    def __create_struct_item(
            self,
            name: str,
            data_type: str,
            type_name: str,
            members: dict,
            ifdef=None,
            functions=None,
            deprecated=False,
    ) -> Item:
        items = dict()
        for member_name, data in members.items():
            items[member_name] = self.__parse_type(name, data)
        return StructItem(name, ifdef, deprecated, items, type_name, functions)

    def __parse_type(self, name: str, data: dict) -> Item:
        item_type = data["data_type"]
        if item_type == "array":
            return self.__create_array_item(name, **data)
        elif item_type == "struct":
            return self.__create_struct_item(name, **data)
        elif item_type == "string":
            def_val = f"\"{data['default']}\""
            return self.__create_string_item(name, "char", data["length"], {
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
            if item.deprecated:
                line = f"case {str(crc)}:\n deprecated_items.{item.name} =  EepromAccess::deserialize_data<{item.get_type_name()}>(data).data;\n return false;"
            else:
                line = f"case {str(crc)}:\n store.{item.name}.init(EepromAccess::deserialize_data<{item.get_type_name()}>(data).data); \n return true;"
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
    parser.add_argument("ImplFile",
                        help="Path to where to store the output file",
                        type=Path)
    # parser.add_argument("Printer", help="Name of the printer", type=str)

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
        file.write("#include \"structure_includes.hpp\"\n")
        file.write("namespace configuration_store{\n")

        file.write("\n".join([
            item.get_additional_declarations() for item in items
            if item.get_additional_declarations() is not None
        ]))
        file.write("\n")

        file.write("struct ConfigurationStoreStructure {\n")
        file.write("private:\n")

        # file.write("protected:\n")
        file.write("public:\n")
        file.write(f"static constexpr size_t NUM_OF_ITEMS = {len(items)};\n")
        file.write("// items\n")

        for item in (item for item in items if not item.deprecated):
            file.write(item.get_item_declaration())
            file.write("\n")

        file.write("auto tuplify(){\n return std::tie(\n")

        if [item
                for item in items if not item.deprecated][0].ifdef is not None:
            file.write(f"{add_ifdef(items[0].ifdef, items[0].name)}\n")
        else:
            file.write(f"{items[0].name}\n")
        for item in [item for item in items if not item.deprecated][1:]:
            line = f",{item.name}"
            if item.ifdef is not None:
                file.write(f"{add_ifdef(item.ifdef, line)}\n")
            else:
                file.write(f"{line}\n")
        file.write("); };\n};\n}\n")

        file.write("struct DeprecatedItems{\n")
        for item in (item for item in items if item.deprecated):
            file.write(
                f"std::optional<{item.get_type_name()}> {item.get_name()} = std::nullopt;\n"
            )
        file.write("};\n")

    with open(args.OutputSwitch, "w") as file:
        file.write("#include \"configuration_store/item_updater.hpp\"\n")
        file.write("#include \"configuration_store/eeprom_access.hpp\"\n")
        file.write("using namespace configuration_store;\n")
        file.write(
            "#include \"configuration_store/configuration_store.hpp\"\n")
        file.write(
            "bool ItemUpdater::operator()(uint32_t crc,const std::vector<uint8_t> &data) {\n"
        )
        file.write("switch(crc){\n")
        for case in item_parser.get_switch_cases():
            file.write(f"{case}\n")

        file.write("default:\n return false;\n")
        file.write("}\n")
        file.write("return false;\n")
        file.write("}\n")

        file.write("void ItemUpdater::updated(uint32_t crc) {\n")
        file.write("switch(crc){\n")
        for item in (item for item in items if item.deprecated):
            file.write(
                f"case {item.crc32}:\n deprecated_items.{item.get_name()} = std::nullopt;\n return;\n"
            )

        file.write("default:\n return;\n")
        file.write("}\n")
        file.write("return;\n")
        file.write("}\n")

    with open(args.ImplFile, "w") as file:
        file.write(
            "#include \"configuration_store/configuration_store.hpp\"\n")
        file.write("using namespace configuration_store;\n")
        file.write("\n".join([
            item.get_additional_definitions() for item in items
            if item.get_additional_definitions() is not None
        ]))
        file.write("\n")


if __name__ == "__main__":
    exit(main())
