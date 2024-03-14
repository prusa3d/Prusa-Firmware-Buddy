/**
 * @file
 */
#include "config_buddy_2209_02.h"
#include "wdt.h"
#include "at21csxx_otp.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include <cmath>
#include <cstring>

#define MAX_RETRY 8

OtpFromEeprom::OtpFromEeprom(GPIO_TypeDef *port, uint32_t pin)
    : eeprom(port, pin) {
    loadData();
}

bool OtpFromEeprom::loadData() {
    // Read data from EEPROM
    wdt_iwdg_refresh();
    status.data_valid = false;
    memset(&calib_data, 0, sizeof(calib_data));

    for (status.retried = 0; status.retried < MAX_RETRY; ++status.retried) {
        if (eeprom.read_block(0, (uint8_t *)&calib_data, sizeof(calib_data)) != 1) {
            wdt_iwdg_refresh();
            continue;
        }

        if (calib_data.size == sizeof(calib_data) && calib_data.datamatrix[0] == '0' && calib_data.datamatrix[1] == '0') { // heuristics: if read failed somehow, these values are probably not OK
            status.data_valid = true;
            break;
        } else {
            wdt_iwdg_refresh();
            eeprom.reset_discovery();
        }
    }

    status.single_read_error_counter = eeprom.get_single_read_error();
    status.repeated_read_error_counter = eeprom.get_repeated_read_error();
    status.cyclic_read_error_counter = eeprom.get_cyclic_read_error();
    return status.data_valid;
}
