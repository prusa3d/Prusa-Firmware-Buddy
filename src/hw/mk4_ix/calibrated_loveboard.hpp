
#pragma once

#include "LoveBoard_EEPROM_Struct.h"
#include "../hw/AT21CSxx.hpp"
#include "crc32.h"

class CalibratedLoveboard {
public:
    CalibratedLoveboard(GPIO_TypeDef *port, uint32_t pin);

    bool dataValid();

    bool loadData();

    struct LoveBoardEeprom calib_data;

private:
    AT21CSxx loveboard_eeprom;
    bool data_valid = false;
};
