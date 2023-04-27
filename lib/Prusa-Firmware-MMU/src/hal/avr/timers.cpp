/// @file timers.cpp
#include "../timers.h"

namespace hal {
namespace timers {

void Configure_CTC8(const uint8_t timer, Tim8bit_TypeDef *const htim8, Tim8bit_CTC_config *const conf) {
    TIMSK[timer] = 0; //clear all interrupt sources
    htim8->TCCRxA = (1 << 1); //WGM=2 (CTC)
    htim8->TCCRxB = conf->cs; //CS = bits from conf (<<0)
    htim8->OCRxA = conf->ocra; //set the TOP value from config
    htim8->TCNTx = 0; //initialize timer to 0
    TIFR[timer] = 0xFF; //clear all interrupt flags
    TIMSK[timer] = (1 << 1); //enable OCIExA
}

}
}
