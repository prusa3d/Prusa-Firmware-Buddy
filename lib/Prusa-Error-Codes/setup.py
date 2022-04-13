# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

from setuptools import setup, find_packages

setup(
    name="prusa_errors",
    version="2021.04.14",
    packages=find_packages(exclude=["tests"]),
    url="https://github.com/prusa3d/Prusa-Error-Codes",
    license="GNU General Public License v3 or later (GPLv3+)",
    classifiers=["License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)"],
    include_package_data=True,
)
