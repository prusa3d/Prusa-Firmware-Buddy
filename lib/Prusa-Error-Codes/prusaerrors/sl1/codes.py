# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

"""
SL1 error codes

Warning: The codes has not yet been officially approved.
"""


from prusaerrors.shared.codes import Code, Category, unique_codes, Codes, Printer


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
    GENERAL_FAILED_TO_MQTT_SEND = Code(PRINTER, Category.CONNECTIVITY, 1, "Cannot send factory config to MQTT!", False)
    GENERAL_NOT_CONNECTED_TO_NETWORK = Code(PRINTER, Category.CONNECTIVITY, 2, None, False)
    GENERAL_CONNECTION_FAILED = Code(PRINTER, Category.CONNECTIVITY, 3, None, False)
    GENERAL_DOWNLOAD_FAILED = Code(PRINTER, Category.CONNECTIVITY, 4, None, False)

    # Electrical
    GENERAL_MOTION_CONTROLLER_EXCEPTION = Code(PRINTER, Category.ELECTRICAL, 6, None, False)
    EXPOSURE_RESIN_SENSOR_FAILURE = Code(PRINTER, Category.ELECTRICAL, 7, None, False)
    GENERAL_NOT_UV_CALIBRATED = Code(PRINTER, Category.ELECTRICAL, 8, None, False)

    # System
    NONE = Code(PRINTER, Category.SYSTEM, 0, "No problem", False)
    UNKNOWN = Code(PRINTER, Category.SYSTEM, 1, "An unknown error has occurred", False)
    EXPOSURE_PROJECT_FAILURE = Code(PRINTER, Category.SYSTEM, 4, None, False)
    GENERAL_CONFIG_EXCEPTION = Code(PRINTER, Category.SYSTEM, 5, "Failed to read configuration file", False)
    GENERAL_NOT_AVAILABLE_IN_STATE = Code(PRINTER, Category.SYSTEM, 6, None, False)
    GENERAL_DBUS_MAPPING_EXCEPTION = Code(PRINTER, Category.SYSTEM, 7, None, False)
    GENERAL_REPRINT_WITHOUT_HISTORY = Code(PRINTER, Category.SYSTEM, 8, None, False)
    GENERAL_MISSING_WIZARD_DATA = Code(PRINTER, Category.SYSTEM, 9, "The wizard did not finish successfully!", False)
    GENERAL_MISSING_CALIBRATION_DATA = Code(
        PRINTER, Category.SYSTEM, 10, "The calibration did not finish successfully!", False
    )
    GENERAL_MISSING_UVCALIBRATION_DATA = Code(
        PRINTER, Category.SYSTEM, 11, "The automatic UV LED calibration did not finish successfully!", False
    )
    GENERAL_MISSING_UVPWM_SETTINGS = Code(PRINTER, Category.SYSTEM, 12, None, False)
    GENERAL_FAILED_UPDATE_CHANNEL_SET = Code(PRINTER, Category.SYSTEM, 13, "Cannot set update channel", False)
    GENERAL_FAILED_UPDATE_CHANNEL_GET = Code(PRINTER, Category.SYSTEM, 14, None, False)
    EXPOSURE_WARNING_ESCALATION = Code(PRINTER, Category.SYSTEM, 15, None, False)
    GENERAL_NOT_ENOUGH_INTERNAL_SPACE = Code(PRINTER, Category.SYSTEM, 16, None, False)
    GENERAL_ADMIN_NOT_AVAILABLE = Code(PRINTER, Category.SYSTEM, 17, None, False)
    GENERAL_FILE_NOT_FOUND = Code(PRINTER, Category.SYSTEM, 18, "Cannot find a file!", False)
    GENERAL_INVALID_EXTENSION = Code(PRINTER, Category.SYSTEM, 19, "File has an invalid extension!", False)
    GENERAL_FILE_ALREADY_EXISTS = Code(PRINTER, Category.SYSTEM, 20, "File already exists!", False)
    GENERAL_INVALID_PROJECT = Code(PRINTER, Category.SYSTEM, 21, "The project file is invalid!", False)

    # Warnings
    NONE_WARNING = Code(PRINTER, Category.WARNINGS, 0, "No warning", False)
    UNKNOWN_WARNING = Code(PRINTER, Category.WARNINGS, 1, "Unknown warning", False)
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
