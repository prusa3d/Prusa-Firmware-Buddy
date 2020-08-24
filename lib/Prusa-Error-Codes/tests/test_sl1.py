# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

# pylint: disable = missing-function-docstring
# pylint: disable = missing-class-docstring
# pylint: disable = missing-module-docstring

import unittest

from prusaerrors.sl1.codes import Sl1Codes


class TestErrors(unittest.TestCase):
    def test_str_conversion(self):
        self.assertEqual("#10500", str(Sl1Codes.NONE))

    def test_code_lookup(self):
        self.assertEqual(Sl1Codes.NONE, Sl1Codes.get("#10500"))

    def test_unknown_code_lookup(self):
        self.assertEqual(Sl1Codes.UNKNOWN, Sl1Codes.get("random string"))


if __name__ == "__main__":
    unittest.main()
