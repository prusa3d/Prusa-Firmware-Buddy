# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

# pylint: disable = missing-function-docstring
# pylint: disable = missing-class-docstring
# pylint: disable = missing-module-docstring

import unittest

from prusaerrors.shared.codes import Printer, Category
from prusaerrors.connect.codes import PrinterCodes


class TestErrors(unittest.TestCase):

    def test_XL(self):
        code = PrinterCodes.get("17505")
        assert code.printer == Printer.XL
        assert code.category == Category(5)
        assert code.error == 5
        assert code.title
        assert code.message
        assert code.id

    def test_MK4(self):
        code = PrinterCodes.get("13505")
        assert code.printer == Printer.MK4
        assert code.category == Category(5)
        assert code.error == 5
        assert code.title
        assert code.message
        assert code.id

    def test_MK4S(self):
        code = PrinterCodes.get("26505")
        assert code.printer == Printer.MK4S
        assert code.category == Category(5)
        assert code.error == 5
        assert code.title
        assert code.message
        assert code.id

    def test_MK39(self):
        code = PrinterCodes.get("21505")
        assert code.printer == Printer.MK39
        assert code.category == Category(5)
        assert code.error == 5
        assert code.title
        assert code.message
        assert code.id

    def test_MK35(self):
        code = PrinterCodes.get("23701")
        assert code.printer == Printer.MK35
        assert code.category == Category(7)
        assert code.error == 1
        assert code.title
        assert code.message
        assert code.id

    def test_MK35S(self):
        code = PrinterCodes.get("28701")
        assert code.printer == Printer.MK35S
        assert code.category == Category(7)
        assert code.error == 1
        assert code.title
        assert code.message
        assert code.id

    def test_no_MK35S(self):
        """MK35S doesn't have puppies."""
        code = PrinterCodes.get("28512")
        assert code is None

    def test_unknown_code(self):
        code = PrinterCodes.get("unknown_code")
        assert code is None


if __name__ == "__main__":
    unittest.main()
