#include "M73_PE.h"
#include "progress_data_wrapper.h"
#include <stdio.h>

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

void print_dur_to_string(char *buffer, uint32_t print_dur) {
    int d = ((print_dur / 60) / 60) / 24,
        h = ((print_dur / 60) / 60) % 24,
        m = (print_dur / 60) % 60,
        s = print_dur % 60;

    if (d) {
        snprintf(buffer, 13, "%3id %2ih %2im", d, h, m);
    } else if (h) {
        snprintf(buffer, 13, "     %2ih %2im", h, m);
    } else if (m) {
        snprintf(buffer, 13, "     %2im %2is", m, s);
    } else {
        snprintf(buffer, 13, "         %2is", s);
    }
}
}
