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

    def test_code_lookup(self):
        code = PrinterCodes.get("17505")
        assert code.printer == Printer(17)
        assert code.category == Category(5)
        assert code.error == 5
        assert code.title
        assert code.message
        assert code.id

    def test_unknown_code(self):
        code = PrinterCodes.get("unknown_code")
        assert code is None


if __name__ == "__main__":
    unittest.main()
