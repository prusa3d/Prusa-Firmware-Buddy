/// @file
#include "voltage.h"
#include "../hal/adc.h"
#include "../logic/error_codes.h"
#include "../panic.h"

namespace modules {
namespace voltage {

VCC vcc;

void VCC::Step() {
    uint16_t tmp;
    // dummy reads are so that the final measurement is valid
    for (uint8_t i = 0; i < config::VCCADCReadCnt; i++) {
        tmp = hal::adc::ReadADC(config::VCCADCIndex);
    }
    vcc_val = tmp;
}

} // namespace voltage
} // namespace modules
