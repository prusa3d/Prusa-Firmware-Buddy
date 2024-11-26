# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

"""
SL1 error codes

Warning: The codes has not yet been officially approved.
"""

import builtins
from pathlib import Path
from re import compile as re_compile

import yaml

from prusaerrors.shared.codes import Category, Code, Printer, unique_codes, Codes
from prusaerrors.sl1 import PRINTER_MODEL_PATH

if "_" not in vars(builtins):

    def _(value):
        return value


def yaml_codes(src_path: Path):
    """
    Add code definitions from YAML source
    """

    def decor(cls):
        with src_path.open("r") as src_file:
            data = yaml.safe_load(src_file)
        assert "Errors" in data

        printer = Printer.UNKNOWN
        model = [x.name for x in PRINTER_MODEL_PATH.iterdir() if x.is_file()]
        if len(model) != 1:
            raise KeyError(f"None or multiple model files found. Check {PRINTER_MODEL_PATH} folder.")

        model = model[0]
        if Printer.M1.name.lower() in model:
            printer = Printer.M1
        elif Printer.SL1.name.lower() in model:
            # this case covers both SL1 and SL1S since they have identical error codes
            printer = Printer.SL1

        re = re_compile(
            r"^(?P<printer>([0-9][0-9]|XX))"
            r"(?P<category>[0-9])"
            r"(?P<error>[0-9][0-9])$"
        )
        for entry in data["Errors"]:
            code_parts = re.match(entry["code"]).groupdict()
            category = Category(int(code_parts["category"]))
            error = int(code_parts["error"])
            printers = entry.get("printers")
            action = entry.get("action", [])

            if code_parts["printer"] == 'XX':
                if printer.name in printers:
                    setattr(
                        cls,
                        entry["id"],
                        Code(
                            printer,
                            category,
                            error,
                            entry["title"],
                            entry["text"],
                            entry.get("approved", False),
                            action
                        )
                    )
            else:
                raise KeyError("current code has specific prefix. It has to be XX...")
        return cls

    return decor


@unique_codes
@yaml_codes(Path(__file__).parent / "errors.yaml")
class Sl1Codes(Codes):
    """
    Error, exception and warning identification codes

    Content is loaded by yaml_codes decorator
    """

    @classmethod
    def get(cls, code: str):
        """
        Get Code by its number

        UNKNOWN Code is received on unknown code number

        :param code: Code number
        :return: Code instance
        """
        try:
            return super().get(code)
        except KeyError:
            return cls.UNKNOWN  # pylint: disable=no-member
