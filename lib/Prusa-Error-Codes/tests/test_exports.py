# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

# pylint: disable = missing-function-docstring
# pylint: disable = missing-class-docstring
# pylint: disable = missing-module-docstring

import json
import unittest
from io import StringIO

from prusaerrors.shared.codes import unique_codes, unique_titles, Codes, Printer, Code, Category


@unique_codes
@unique_titles
class TestCodes(Codes):
    PRINTER = Printer.UNKNOWN
    NONE = Code(PRINTER, Category.SYSTEM, 0, "No title", "No problem", True)


class TestErrors(unittest.TestCase):
    def test_json_export(self):
        sio = StringIO()
        TestCodes.dump_json(sio)

        print("JSON:")
        print(sio.getvalue())

        sio.seek(0)
        obj = json.load(sio)
        self.assertIn("none", obj)

    def test_cpp_messages_export(self):
        sio = StringIO()
        TestCodes.dump_cpp_messages(sio)

        print("C++ messages:")
        print(sio.getvalue())

        self.assertRegex(sio.getvalue(), r'00500, "No problem",')

    def test_cpp_ts_export(self):
        sio = StringIO()
        TestCodes.dump_cpp_ts(sio)

        print("C++ ts:")
        print(sio.getvalue())

        self.assertRegex(sio.getvalue(), r'QT_TR_NOOP\("No problem"\);')

    def test_qml_dict_export(self):
        sio = StringIO()
        TestCodes.dump_qml_dictionary(sio)

        print("QML dict:")
        print(sio.getvalue())

        self.assertRegex(sio.getvalue(), r'500: qsTr\("No problem"\)')

    def test_str_conversion(self):
        self.assertEqual("#00500", str(TestCodes.NONE))

    def test_code_lookup(self):
        self.assertEqual(TestCodes.NONE, TestCodes.get("#00500"))

    def test_unknown_code_lookup(self):
        with self.assertRaises(KeyError):
            TestCodes.get("random string")

    def test_google_docs(self):
        sio = StringIO()
        TestCodes.dump_google_docs(sio)
        print("Google docs text:")
        print(sio.getvalue())
        self.assertRegex(sio.getvalue(), r'SL1\t10\tSystem errors\t5\t0\t"NONE"\t"No problem"\t#00500')


if __name__ == "__main__":
    unittest.main()
