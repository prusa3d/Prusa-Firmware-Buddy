# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

"""
SL1 error codes

Warning: The codes has not yet been officially approved.
"""

from prusaerrors.shared.codes import Code, Category, unique_codes, Codes, Printer

try:
    _
except NameError:
    _ = lambda x: x


@unique_codes
class Sl1Codes(Codes):
    """
    Error, exception and warning identification codes
    """

    PRINTER = Printer.SL1

    # Mechanical
    GENERAL_TILT_HOME_FAILURE = Code(PRINTER, Category.MECHANICAL, 1, None, False)
    GENERAL_TOWER_HOME_FAILURE = Code(PRINTER, Category.MECHANICAL, 2, None, False)
    EXPOSURE_TOWER_MOVE_FAILURE = Code(PRINTER, Category.MECHANICAL, 3, None, False)
    EXPOSURE_FAN_FAILURE = Code(PRINTER, Category.MECHANICAL, 6, None, False)
    EXPOSURE_RESIN_TOO_LOW = Code(PRINTER, Category.MECHANICAL, 8, None, False)
    EXPOSURE_RESIN_TOO_HIGH = Code(PRINTER, Category.MECHANICAL, 9, None, False)
    EXPOSURE_TILT_FAILURE = Code(PRINTER, Category.MECHANICAL, 10, None, False)
    EXPOSURE_TOWER_FAILURE = Code(PRINTER, Category.MECHANICAL, 12, None, False)
    GENERAL_NOT_MECHANICALLY_CALIBRATED = Code(PRINTER, Category.MECHANICAL, 13, None, False)

    # Temperature
    EXPOSURE_TEMP_SENSOR_FAILURE = Code(PRINTER, Category.TEMPERATURE, 5, None, False)

    # Connectivity
    GENERAL_FAILED_TO_MQTT_SEND = Code(
        PRINTER, Category.CONNECTIVITY, 1, _("Cannot send factory config to MQTT!"), False
    )
    GENERAL_NOT_CONNECTED_TO_NETWORK = Code(PRINTER, Category.CONNECTIVITY, 2, None, False)
    GENERAL_CONNECTION_FAILED = Code(PRINTER, Category.CONNECTIVITY, 3, None, False)
    GENERAL_DOWNLOAD_FAILED = Code(PRINTER, Category.CONNECTIVITY, 4, None, False)

    # Electrical
    MOTION_CONTROLLER_WRONG_REVISION = Code(
        PRINTER,
        Category.ELECTRICAL,
        1,
        _("Wrong revision of the motion controller. Please replace it or contact our support."),
        False,
    )
    GENERAL_MOTION_CONTROLLER_EXCEPTION = Code(PRINTER, Category.ELECTRICAL, 6, None, False)
    EXPOSURE_RESIN_SENSOR_FAILURE = Code(PRINTER, Category.ELECTRICAL, 7, None, False)
    GENERAL_NOT_UV_CALIBRATED = Code(PRINTER, Category.ELECTRICAL, 8, None, False)

    # System
    NONE = Code(PRINTER, Category.SYSTEM, 0, _("No problem"), False)
    UNKNOWN = Code(
        PRINTER,
        Category.SYSTEM,
        1,
        _(
            "An unexpected error has occurred :-(.\n\n"
            "If the SL1 is printing, current job will be finished.\n\n"
            "You can turn the printer off by pressing the front power button.\n\n"
            "Please follow the instructions in Chapter 3.1 in the handbook to learn how to save a log file. "
            "Please send the log to us and help us improve the printer.\n\n"
            "Thank you!"
        ),
        False,
    )
    EXPOSURE_PROJECT_FAILURE = Code(PRINTER, Category.SYSTEM, 4, None, False)
    GENERAL_CONFIG_EXCEPTION = Code(PRINTER, Category.SYSTEM, 5, _("Failed to read configuration file"), False)
    GENERAL_NOT_AVAILABLE_IN_STATE = Code(PRINTER, Category.SYSTEM, 6, None, False)
    GENERAL_DBUS_MAPPING_EXCEPTION = Code(PRINTER, Category.SYSTEM, 7, None, False)
    GENERAL_REPRINT_WITHOUT_HISTORY = Code(PRINTER, Category.SYSTEM, 8, None, False)
    GENERAL_MISSING_WIZARD_DATA = Code(PRINTER, Category.SYSTEM, 9, _("The wizard did not finish successfully!"), False)
    GENERAL_MISSING_CALIBRATION_DATA = Code(
        PRINTER, Category.SYSTEM, 10, _("The calibration did not finish successfully!"), False
    )
    GENERAL_MISSING_UVCALIBRATION_DATA = Code(
        PRINTER, Category.SYSTEM, 11, _("The automatic UV LED calibration did not finish successfully!"), False
    )
    GENERAL_MISSING_UVPWM_SETTINGS = Code(PRINTER, Category.SYSTEM, 12, None, False)
    GENERAL_FAILED_UPDATE_CHANNEL_SET = Code(PRINTER, Category.SYSTEM, 13, _("Cannot set update channel"), False)
    GENERAL_FAILED_UPDATE_CHANNEL_GET = Code(PRINTER, Category.SYSTEM, 14, None, False)
    EXPOSURE_WARNING_ESCALATION = Code(PRINTER, Category.SYSTEM, 15, None, False)
    GENERAL_NOT_ENOUGH_INTERNAL_SPACE = Code(PRINTER, Category.SYSTEM, 16, None, False)
    GENERAL_ADMIN_NOT_AVAILABLE = Code(PRINTER, Category.SYSTEM, 17, None, False)
    GENERAL_FILE_NOT_FOUND = Code(PRINTER, Category.SYSTEM, 18, _("Cannot find a file!"), False)
    GENERAL_INVALID_EXTENSION = Code(PRINTER, Category.SYSTEM, 19, _("File has an invalid extension!"), False)
    GENERAL_FILE_ALREADY_EXISTS = Code(PRINTER, Category.SYSTEM, 20, _("File already exists!"), False)
    GENERAL_INVALID_PROJECT = Code(PRINTER, Category.SYSTEM, 21, _("The project file is invalid!"), False)

    # Warnings
    NONE_WARNING = Code(PRINTER, Category.WARNINGS, 0, _("No warning"), False)
    UNKNOWN_WARNING = Code(PRINTER, Category.WARNINGS, 1, _("Unknown warning"), False)
    EXPOSURE_AMBIENT_TOO_HOT_WARNING = Code(PRINTER, Category.WARNINGS, 2, None, False)
    EXPOSURE_AMBIENT_TOO_COLD_WARNING = Code(PRINTER, Category.WARNINGS, 3, None, False)
    EXPOSURE_PRINTING_DIRECTLY_WARNING = Code(PRINTER, Category.WARNINGS, 4, None, False)
    EXPOSURE_PRINTER_MODEL_MISMATCH_WARNING = Code(PRINTER, Category.WARNINGS, 5, None, False)
    EXPOSURE_RESIN_NOT_ENOUGH_WARNING = Code(PRINTER, Category.WARNINGS, 6, None, False)
    EXPOSURE_PROJECT_SETTINGS_MODIFIED_WARNING = Code(PRINTER, Category.WARNINGS, 7, None, False)

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
            return cls.UNKNOWN
