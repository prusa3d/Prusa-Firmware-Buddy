import pytest
from structure_generator import ItemParser, Item, BasicItem, ArrayItem, StructItem


def test_array_def_for_each():
    data = {
        "data_type": "array",
        "length": 10,
        "item": {
            "data_type": "int32_t",
            "default": 0
        }
    }

    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == ArrayItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == "MemConfigItem<std::array<int32_t,10>> test { \"test\" , 0 };"


def test_string():
    data = {"data_type": "string", "length": 100, "default": "test_data"}
    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == ArrayItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == "MemConfigItem<std::array<char,100>> test { \"test\" , \"test_data\" };"


def test_bool():
    data = {"data_type": "bool", "default": True}
    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == BasicItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == "MemConfigItem<bool> test { \"test\" , true };"
    data["default"] = False
    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == BasicItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == "MemConfigItem<bool> test { \"test\" , false };"


def test_number():
    data = {"data_type": "int32_t", "default": 0}
    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == BasicItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == "MemConfigItem<int32_t> test { \"test\" , 0 };"


def test_struct():
    data = {
        "data_type": "struct",
        "type_name": "Struct",
        "members": {
            "array": {
                "data_type": "array",
                "length": 10,
                "item": {
                    "data_type": "int32_t",
                    "default": 0
                }
            },
            "SimpleType": {
                "data_type": "int32_t",
                "default": 0
            },
            "string_type": {
                "data_type": "string",
                "default": "test string",
                "length": 100
            }
        }
    }
    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == StructItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == 'MemConfigItem<Struct> test { "test" , { 0, 0, "test string" } };'
    assert item.get_additional_definitions() == \
           """struct Struct {
std::array<int32_t,10> array;
int32_t SimpleType;
std::array<char,100> string_type;

template <class Packer>
void pack (Packer &pack) {
pack( array, SimpleType, string_type); }
};"""


def test_ifdef():
    data = {
        "data_type": "int32_t",
        "default": 0,
        "ifdef": "defined(HAS_LIVE_Z)"
    }
    item_parser = ItemParser()
    item = item_parser.parse_type("test", data)
    assert type(item) == BasicItem
    assert item.name == "test"
    assert item.get_item_declaration(
    ) == "#if defined(HAS_LIVE_Z)\n MemConfigItem<int32_t> test { \"test\" , 0 };\n#endif"
