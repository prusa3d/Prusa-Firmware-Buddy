# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

"""
SL1 error codes

Warning: The codes has not yet been officially approved.
"""

import builtins
from pathlib import Path

from prusaerrors.shared.codes import unique_codes, Codes, yaml_codes

if "_" not in vars(builtins):

    def _(value):
        return value


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
