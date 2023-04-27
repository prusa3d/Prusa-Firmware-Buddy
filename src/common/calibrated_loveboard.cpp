/**
 * @file
*/
#include "config_buddy_2209_02.h"
#include "wdt.h"
#include "calibrated_loveboard.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include <cmath>
#include <cstring>

CalibratedLoveboard::CalibratedLoveboard(GPIO_TypeDef *port, uint32_t pin) {

    loveboard_eeprom = new AT21CSxx(port, pin);
    loadData();
#ifndef _DEBUG
    delete loveboard_eeprom;
#endif
}

bool CalibratedLoveboard::loadData() {
    //Read data from EEPROM
    wdt_iwdg_refresh();
    data_valid = false;
    memset(&calib_data, 0, sizeof(calib_data));

    for (int i = 0; i < MAX_CRC_RETRY; i++) {
        for (int j = 0; j < MAX_READ_RETRY; j++) {
            if (loveboard_eeprom->read_block(0, (uint8_t *)&calib_data, sizeof(calib_data)) == 1) {
                data_valid = 1;
                break;
            }
            wdt_iwdg_refresh();
        }

        if (calib_data.struct_size != sizeof(calib_data) || calib_data.datamatrix_id[0] != '0' || calib_data.datamatrix_id[1] != '0') { // heuristics: if read failed somehow, these values are probably not OK
            data_valid = 0;
            wdt_iwdg_refresh();
        } else {
            break;
        }
        wdt_iwdg_refresh();
        loveboard_eeprom->reset_discovery();
    }
    return data_valid;
}

bool CalibratedLoveboard::dataValid() {
    return data_valid;
}

CalibratedLoveboard::~CalibratedLoveboard() {
#ifdef _DEBUG
    delete loveboard_eeprom;
#endif
}
