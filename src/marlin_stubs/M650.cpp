#include <device/board.h>
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM && defined LOVEBOARD_HAS_PT100)

    #include "../../lib/Marlin/Marlin/src/gcode/queue.h"
    #include "PrusaGcodeSuite.hpp"
    #include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
    #include "../calibrated_loveboard.hpp"
    #include <cstring>
    #include "../common/hwio.h"

void printEEPROMData() {
    he_therm_cal data;
    memcpy(&data, &LoveBoard->calib_data, sizeof(he_therm_cal));
    int buf_point = 0;
    char buffer[200];
    if (!LoveBoard->dataValid()) {
        buf_point += snprintf(buffer, sizeof(buffer), "INVALID DATA FOUND!!!\n");
        buf_point -= 1;
    }

    buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "A: %u\nSerial: ", data.amplification);
    for (int i = 7; i >= 0; i--) {
        buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "%02x", (int)data.serial_number[i]);
        if (i == 2 || i == 4 || i == 6) {
            buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "-");
        }
    }
    buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "\n");

    int max = data.point_count;
    if (max > 5) {
        max = 5;
    }
    for (int i = 0; i < max; i++) {
        int resistance = (int)(data.calib_data[i].resistance * 1000);
        int hxData = data.calib_data[i].hx_data;
        buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "P%i:(%i, %i)  \n", i, resistance, hxData);
    }
    if (data.point_count > 5) {
        buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "...\n");
    }
    buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "CRC: %lu\n", data.crc);
    #ifdef _DEBUG
    buf_point += snprintf(buffer + buf_point, sizeof(buffer) - buf_point, "EEPROM read error count: %i", LoveBoard->loveboard_eeprom->error_counter);
    #endif
    SERIAL_ECHOLN(buffer);
}

void printCalculatedResistance() {
    char buffer[30] = { 0 };
    snprintf(buffer, sizeof(buffer) - 1, "PT100 resistance %f", (double)hwio_get_hotend_resistance());
    SERIAL_ECHOLN(buffer);
}

void PrusaGcodeSuite::M650() {

    if (!parser.seen_any()) { //No command specified, just print data.
        printEEPROMData();
        return;
    }

    #ifdef _DEBUG           //Writing to eeprom only available in debug
    if (parser.seen('A')) { //Set amplification used for measurement
        int amp = parser.intval('A');
        if (amp == 8 || amp == 64) {
            LoveBoard->calib_data.amplification = amp;
            SERIAL_ECHOLN("Amplification set");
        } else {
            SERIAL_ECHOLN("Unknown amplification value");
        }
    }

    if (parser.seen('S')) { //Set serial number. TODO: Serial is limited by parser intval(16 bit). Find way to use all 64bits of serial number.
        uint64_t serial = (uint64_t)parser.intval('S');
        memcpy(LoveBoard->calib_data.serial_number, &serial, 8);
        SERIAL_ECHOLN("Serial set");
    }

    if (parser.seen('R')) { //Write data to calibration table/print PT100 resistance
        if (parser.seen('P') && parser.intval('P') < MAX_CALIB_POINTS) {
            int data = hwio_get_hotend_temp_raw() / 256;
            LoveBoard->calib_data.calib_data[parser.intval('P')].hx_data = data;
            LoveBoard->calib_data.calib_data[parser.intval('P')].resistance = parser.floatval('R');
            char buffer[30] = { 0 };
            snprintf(buffer, sizeof(buffer), "Datapoint set: %i", data);
            SERIAL_ECHOLN(buffer);
        } else {
            printCalculatedResistance();
        }
    }

    if (parser.seen('W')) { //Write current calibration table to eeprom
        LoveBoard->calib_data.struct_ver = 1;
        LoveBoard->calib_data.struct_size = sizeof(he_therm_cal);
        LoveBoard->calib_data.point_count = 3;
        LoveBoard->calib_data.hw_rev[0] = 2;
        LoveBoard->calib_data.hw_rev[1] = 0;
        LoveBoard->calib_data.hw_rev[2] = 0;
        LoveBoard->calib_data.crc = 0;
        LoveBoard->calib_data.calib_date[0] = 0;
        LoveBoard->calib_data.calib_date[1] = 0;
        LoveBoard->calib_data.calib_date[2] = 0;
        LoveBoard->calib_data.calib_station_id = 0;

        uint32_t crc = crc32_calc((uint8_t *)&(LoveBoard->calib_data), sizeof(he_therm_cal));
        LoveBoard->calib_data.crc = crc;

        int res = LoveBoard->loveboard_eeprom->write_block(0, (uint8_t *)&LoveBoard->calib_data, sizeof(he_therm_cal));
        char buffer[30] = { 0 };
        snprintf(buffer, sizeof(buffer), "EEPROM write %s", res ? "success" : "fail");
        SERIAL_ECHOLN(buffer);
    }
    #else
    if (parser.seen('R')) { //Print PT100 resistance
        printCalculatedResistance();
    }
    #endif //_DEBUG

    if (parser.seen('H')) { //Print HX717 output(16bit)
        char buffer[40] = { 0 };
        snprintf(buffer, sizeof(buffer), "HX717 raw data(int16_t): %i", (int)hwio_get_hotend_temp_raw() / 256);
        SERIAL_ECHOLN(buffer);
    }

    if (parser.seen('L')) { //Load data from EEPROM
        LoveBoard->loadData();
        SERIAL_ECHOLN("Eeprom loaded");
        printEEPROMData();
    }
}

#endif //(BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM && defined LOVEBOARD_HAS_PT100)
