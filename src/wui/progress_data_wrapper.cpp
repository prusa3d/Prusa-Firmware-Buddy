#include "../../../lib/Marlin/Marlin/src/gcode/lcd/M73_PE.h"
#include "progress_data_wrapper.h"

extern "C" {

bool is_percentage_valid(uint32_t print_dur) {
    return oProgressData.oPercentDone.mIsActual(print_dur);
}

uint8_t progress_get_percentage() {
    return (uint8_t)oProgressData.oPercentDone.mGetValue();
}

void progress_format_time2end(char *dest, uint16_t feedrate) {
    return oProgressData.oTime2End.mFormatSeconds(dest, feedrate);
}
}
