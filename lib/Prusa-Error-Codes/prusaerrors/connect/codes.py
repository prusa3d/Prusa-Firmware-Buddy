# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later
"""
All printer type error/attention codes

Warning: The codes might not have yet been officially approved.
"""
import re
from pathlib import Path
from typing import Optional

import yaml
from prusaerrors.shared.codes import (Category, Code, Codes, Printer,
                                      unique_codes)

BUDDY = [
    'MINI', 'MK4', 'IX', 'XL', 'MK3.5', 'MK4S', 'MK3.9', 'MK3.9S', 'MK3.5S',
    'COREONE'
]


class PrinterCode(Code):
    """
    Code class holds error code information

    """

    # pylint: disable = too-many-arguments
    # pylint: disable = too-many-positional-arguments
    def __init__(
        self,
        printer: Printer,
        category: Category,
        error: int,
        title: str,
        message: str,
        approved: bool,
        id_: str,
    ):
        super().__init__(printer=printer,
                         category=category,
                         error=error,
                         title=title,
                         message=message,
                         approved=approved)
        self.id = id_

    @property
    def code(self) -> str:
        """
        Get error code

        :return: Error code
        """
        return f"x{self.raw_code}"


def yaml_codes(src_path: Path):
    """
    Add code definitions from YAML source
    """

    def decor(cls):
        with src_path.open("r") as src_file:
            data = yaml.safe_load(src_file)
        assert "Errors" in data

        code_re = re.compile(r"^(?P<printer>([0-9][0-9]|XX))"
                             r"(?P<category>[0-9])"
                             r"(?P<error>[0-9][0-9])$")
        for entry in data["Errors"]:
            code_parts = code_re.match(entry["code"]).groupdict()
            category = Category(int(code_parts["category"]))
            error = int(code_parts["error"])

            # code is common for printers defined in printers attribute
            if code_parts["printer"] == 'XX':
                if printers := entry.get("printers"):
                    if 'MK4' in printers:
                        printers.extend(('MK4S', 'MK3.9', 'MK3.9S'))
                    elif 'MK3.5' in printers:
                        printers.append('MK3.5S')
                else:  # if no printers specified code is valid for all buddy
                    printers = BUDDY

                for printer in printers:
                    printer = Printer[printer.upper().replace(".", "")]
                    code = PrinterCode(printer, category, error,
                                       entry["title"], entry["text"],
                                       entry.get("approved",
                                                 False), entry["id"])
                    setattr(cls, str(code), code)

            # code contains printer number
            else:
                printer = Printer(int(code_parts["printer"]))
                code = PrinterCode(printer, category, error,
                                   entry["title"], entry["text"],
                                   entry.get("approved", False), entry["id"])
                setattr(cls, str(code), code)
        return cls

    return decor


@unique_codes
@yaml_codes(Path(__file__).parent.parent / "sl1" / "errors.yaml")
@yaml_codes(Path(__file__).parent.parent / "mmu" / "errors.yaml")
@yaml_codes(Path(__file__).parent.parent / "buddy" / "errors.yaml")
class PrinterCodes(Codes):
    """
    Load all the printer type error, exception and warning identification codes

    Content is loaded by yaml_codes decorators.
    """

    @classmethod
    def get(cls, code: str) -> Optional[PrinterCode]:
        """
        Get Code by its number

        :param code: Code number as string
        :return: Code instance
        """
        try:
            return super().get(f"x{code}")
        except KeyError:
            return None
