# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later
# pylint: disable = missing-function-docstring
# pylint: disable = missing-class-docstring
# pylint: disable = missing-module-docstring
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import MagicMock, patch
from importlib import reload
from prusaerrors.shared.codes import Code


class TestErrors(unittest.TestCase):
    def _test_code(self, printer_model_file: str, code_id: str, code: str) -> Code:
        # create docstring documentation for this method in google style
        """
        Tests the code lookup for a given printer model.
        Sl1Codes relies on the PRINTER_MODEL_PATH to find the correct model file.
        Printer model file is lowercase short name of
        the printer. E.g. sl1, sl1s, m1.

        :param printer_model_file: name of the printer model file
        :param code_id: code id defined in yaml file. E.g. "TILT_HOME_FAILED"
        :param code: code string with printer prefix and leading hash. E.g. "#29101"
        :raises AttributeError: generated code does not match expected code
        """
        with patch("prusaerrors.sl1.PRINTER_MODEL_PATH", new_callable=MagicMock, spec=Path) as mock:
            with TemporaryDirectory() as temp:
                full_path = temp + "/" + printer_model_file
                mock.iterdir.return_value = [Path(full_path)]

                # we need to actually create a file so is_file() returns True
                with open(full_path, "w+", encoding="utf-8") as _:
                    import prusaerrors.sl1.codes
                    reload(prusaerrors.sl1.codes)
                    from prusaerrors.sl1.codes import Sl1Codes
                    generated_code = getattr(Sl1Codes, code_id)
                    self.assertEqual(generated_code, Sl1Codes.get(code))
                    return generated_code

    def test_str_conversion(self):
        code = self._test_code("sl1", "NONE", "#10500")
        self.assertEqual("#10500", str(code))

    def test_code_lookup_sl1(self):
        self._test_code("sl1", "NONE", "#10500")

    def test_code_lookup_sl1s(self):
        self._test_code("sl1s", "NONE", "#10500")

    def test_code_lookup_m1(self):
        self._test_code("m1", "NONE", "#29500")

    def test_unknown_code_lookup(self):
        self._test_code("sl1", "UNKNOWN", "random string")


if __name__ == "__main__":
    unittest.main()
