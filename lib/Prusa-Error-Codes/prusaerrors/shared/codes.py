# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Base classes for SL1 errors and warnings
"""

import functools
import json
import re
from enum import IntEnum, unique
from pathlib import Path
from typing import Dict, List, TextIO, Optional

import yaml


@unique
class Printer(IntEnum):
    """
    Prusa printer code

    These are USB PID codes
    """

    UNKNOWN = 0
    MMU = 0x0004
    SL1 = 0x000A
    MINI = 0x000C
    MK4 = 0x000D
    IX = 0x0010
    XL = 0x0011
    MK39 = 0x0015
    MK35 = 0x0017
    MK4S = 0x001A
    MK39S = 0x001B
    MK35S = 0x001C
    M1 = 0x001D


@unique
class Category(IntEnum):
    """
    Prusa error category codes

    This mapping is taken from general Prusa guidelines on errors, do not
    modify.
    """

    MECHANICAL = 1  # Mechanical failures, engines XYZ, tower
    TEMPERATURE = 2  # Temperature measurement, thermistors, heating
    ELECTRICAL = 3  # Electrical, MINDA, FINDA, Motion Controller, …
    CONNECTIVITY = 4  # Connectivity - Wi - Fi, LAN, Prusa Connect Cloud
    SYSTEM = 5  # System - BSOD, ...
    BOOTLOADER = 6  #
    WARNINGS = 7  # Category-less warnings
    DIALOGS = 8  # Remote Dialogs
    UNKNOWN = 9


@functools.total_ordering
class Code:
    """
    Code class holds error code information
    """

    # pylint: disable = too-many-arguments
    # pylint: disable = too-many-positional-arguments
    def __init__(self,
                 printer: Printer,
                 category: Category,
                 error: int,
                 title: str,
                 message: str,
                 approved: bool,
                 action: Optional[List[str]] = None):
        if printer.value < 0 or printer.value > 99:
            raise ValueError(f"Printer class {printer} out of range")
        if category.value < 0 or category.value > 9:
            raise ValueError(f"Error class {category} out of range")

        if error < 0 or error > 99:
            raise ValueError(f"Error code {error} out of range")

        self._printer = printer
        self._category = category
        self._error = error
        self._title = title
        self._message = message
        self._approved = approved
        self._action = action

    @property
    def code(self) -> str:
        """
        Get error code

        :return: Error code
        """
        return f"#{self.raw_code}"

    @property
    def raw_code(self) -> str:
        """
        Get raw code without "#" at the beginning

        :return: Error code without "#"
        """
        return f"{self._printer.value:02}{self._category.value}{self._error:02}"

    @property
    def raw_message(self) -> str:
        """
        Get raw message with escaped characters (do not translate them)

        :return: Error message with backslash characters
        """
        return json.dumps(self.message)

    @property
    def printer(self) -> Printer:
        """
        Get error code printer

        :return: Error printer enum instance
        """
        return self._printer

    @property
    def error(self) -> int:
        """
        Get error code valid inside a category and printer

        :return: error code integer value
        """
        return self._error

    @property
    def category(self) -> Category:
        """
        Ger error category

        :return: Error category enum instance
        """
        return self._category

    @property
    def title(self) -> str:
        """
        Get error title

        :return: Error category title
        """
        return self._title

    @property
    def message(self) -> str:
        """
        Get error message

        :return: Error message string
        """
        return self._message

    @property
    def approved(self):
        """
        Whenever the message text was approved for use in production system

        Unapproved tests are not supposed to be translated. This is planed
        to raise warnings and prevent the resulting build from being used in
        production.
        """
        return self._approved

    @property
    def action(self) -> List[str]:
        """
        List of strings with actions to be taken when the error occurs.
        """
        return self._action

    def __lt__(self, other):
        if not isinstance(other, Code):
            return NotImplementedError()
        return self.code < other.code

    def __eq__(self, other):
        if not isinstance(other, Code):
            return NotImplementedError()
        return self.code == other.code

    def __repr__(self):
        return (
            f"Code: {self.code} = {str(self.printer)}:{str(self.category)}:{self.error} - {self._title}:{self.message}"
        )

    def __str__(self):
        return self.code


class Codes:
    """
    Base class for code listing classes
    """

    _code_map: Dict[str, Code] = {}

    PRINTER = Printer.UNKNOWN

    @classmethod
    def get_codes(cls) -> Dict[str, Code]:
        """
        Get dictionary containing member codes

        :return: Member code dict
        """
        return {
            item: var
            for item, var in vars(cls).items() if isinstance(var, Code)
        }

    @classmethod
    def get(cls, code: str):
        """
        Get Code by its number

        KeyError is raised when code does not exists

        :param code: Code number
        :return: Code instance
        """
        if not cls._code_map:
            cls._code_map = {
                code.code: code
                for code in cls.get_codes().values()
            }
        return cls._code_map[code]

    @classmethod
    def dump_json(cls, file: TextIO) -> None:
        """
        Dump codes JSON representation to an open file

        :param file: Where to dump
        :return: None
        """
        obj = {
            name.lower(): {
                "code": code.code,
                "message": code.message
            }
            for name, code in cls.get_codes().items()
        }
        return json.dump(obj, file, indent=True)

    @classmethod
    def dump_cpp_enum(cls, file: TextIO) -> None:
        """
        Dump codes C++ enum representation to an open file

        :param file: Where to dump
        :return: None
        """
        file.write("// Generated error code enum\n")
        file.write("namespace ErrorCodes {\n")
        file.write("\tenum Errors {\n")

        for name, code in cls.get_codes().items():
            file.write(f"\t\t{name} = {code.raw_code},\n")

        file.write("\t};\n")
        file.write("};\n")

    @classmethod
    def dump_cpp_messages(cls, file: TextIO) -> None:
        """
        Dump code messages C++ QMap representation to an open file

        :param file: Where to dump
        :return: None
        """
        file.write("#include <QMap>\n")
        file.write("// Generated error code to message mapping\n")
        file.write("static QMap<int, QString> error_messages{\n")

        for code in cls.get_codes().values():
            if code.message and code.approved:
                # file.write('\t{"' + code.code + '", "' + code.message + '"},\n')
                file.write(f"\t{code.raw_code}, {code.raw_message},\n")

        file.write("};\n")

    @classmethod
    def dump_qml_dictionary(cls, file: TextIO):
        """
         Dump code QML map representation to an open file
        :param file: Where to dump
        :return: None
        """
        file.write("import QtQuick 2.10\n")
        file.write(
            "/* Generated by sla-errors. Your edits to this file will be lost. */\n"
        )
        file.write("pragma Singleton\n")
        file.write("Item {\n")
        file.write("\treadonly property var messages:{\n")

        for code in cls.get_codes().values():
            if code.message:
                file.write(f"\t\t{code.raw_code}: qsTr({code.raw_message}),\n")

        file.write("\t}\n")
        file.write("}\n")

    @classmethod
    def dump_cpp_ts(cls, file: TextIO):
        """
        Dump C++ code defining code message translations to an open file

        :param file: Where to dump
        :return: None
        """
        file.write(
            "// Generated translation string definitions for all defined error messages\n"
        )
        for code in cls.get_codes().values():
            if code.message:
                file.write(f"QT_TR_NOOP({code.raw_message});\n")

    @classmethod
    def dump_google_docs(cls, file: TextIO) -> None:
        """
        Dump textual representation of error codes suitable for copy-paste to Google Docs

        :param file: Where to dump
        :return: None
        """
        c2docs = {
            Category.SYSTEM: "System errors",
            Category.MECHANICAL: "Mechanical",
            Category.ELECTRICAL: "Electronics",
            Category.CONNECTIVITY: "Connectivity",
            Category.TEMPERATURE: "Temperatures",
            Category.WARNINGS: "Warnings",
        }

        for name, code in cls.get_codes().items():
            message = code.message if code.message else ""
            category = f"{c2docs[code.category]}\t{code.category.value}"
            file.write(
                f'SL1\t10\t{category}\t{code.error}\t"{name}"\t"{message}"\t{code.code}\n'
            )

    @classmethod
    def dump_yaml(cls, file: TextIO) -> None:
        """
        Dump YAML representation of error codes suitable for Codes class creation using a decorator.

        :param file: Where to dump
        :return: None
        """
        codes = []

        for name, code in cls.get_codes().items():
            codes.append({
                "code": code.raw_code,
                "title": name,
                "text": code.message,
                "id": name,
                "approved": code.approved,
            })

        yaml.dump({"Errors": codes}, file, sort_keys=False)


def unique_codes(cls):
    """
    Class decorator requiring unique code values definition inside the class

    :param cls: Codes class
    :return: Unmodified input class
    """
    used = set()
    for name, code in cls.get_codes().items():
        if code.code in used:
            raise ValueError(
                f"Code {name} with value {code.code} is duplicate!")
        used.add(code.code)

    return cls


def unique_titles(cls):
    """
    Class decorator requiring unique title definition inside the class

    :param cls: Codes class
    :return: Unmodified input class
    """
    used = set()
    for name, code in cls.get_codes().items():
        if code.title in used:
            raise ValueError(
                f"Code {name} with title {code.title} is duplicate!")
        used.add(code.title)

    return cls


def yaml_codes(src_path: Path):
    """
    Add code definitions from YAML source
    """

    def decor(cls):
        with src_path.open("r") as src_file:
            data = yaml.safe_load(src_file)
        assert "Errors" in data

        code_re = re.compile(
            r"^(?P<printer>[0-9][0-9])(?P<category>[0-9])(?P<error>[0-9][0-9])$"
        )
        for entry in data["Errors"]:
            code_parts = code_re.match(entry["code"]).groupdict()
            printer = Printer(int(code_parts["printer"]))
            category = Category(int(code_parts["category"]))
            error = int(code_parts["error"])
            action = entry.get("action", [])
            setattr(
                cls, entry["id"],
                Code(printer, category, error, entry["title"], entry["text"],
                     entry["approved"], action))
        return cls

    return decor
