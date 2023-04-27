
#pragma once

#include "LoveBoard_EEPROM_Struct.h"
#include "../hw/AT21CSxx.hpp"
#include "crc32.h"

#define MAX_CALIB_POINTS 3
#define MAX_READ_RETRY   20
#define MAX_CRC_RETRY    20

class CalibratedLoveboard {

public:
    CalibratedLoveboard(GPIO_TypeDef *port, uint32_t pin);
    virtual ~CalibratedLoveboard();

    bool dataValid();

    bool loadData();
#ifdef _DEBUG
    AT21CSxx *loveboard_eeprom;
#endif

    struct loveboard_eeprom calib_data;

private:
#ifndef _DEBUG
    AT21CSxx *loveboard_eeprom;
#endif
    bool data_valid = false;
};

extern CalibratedLoveboard *LoveBoard;
