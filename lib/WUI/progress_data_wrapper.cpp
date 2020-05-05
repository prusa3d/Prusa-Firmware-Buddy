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

uint32_t progress_time2end(uint16_t feedrate) {
    return (oProgressData.oTime2End.mGetValue()*100)/feedrate;
}

void print_dur_to_string(char *buffer, size_t buffer_len, uint32_t print_dur) {
    int d = ((print_dur / 60) / 60) / 24,
        h = ((print_dur / 60) / 60) % 24,
        m = (print_dur / 60) % 60,
        s = print_dur % 60;

    if (d) {
        snprintf(buffer, buffer_len, "%3id %2ih %2im", d, h, m);
    } else if (h) {
        snprintf(buffer, buffer_len, "     %2ih %2im", h, m);
    } else if (m) {
        snprintf(buffer, buffer_len, "     %2im %2is", m, s);
    } else {
        snprintf(buffer, buffer_len, "         %2is", s);
    }
}
}
