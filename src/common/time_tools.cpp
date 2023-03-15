// time_tools.cpp

#include "time_tools.hpp"
#include "eeprom.h"

static bool time_format_changed = true;

static time_format::TF_t &GetTimeFormat() {
    static time_format::TF_t time_format = (time_format::TF_t)eeprom_get_var(EEVAR_TIME_FORMAT) == time_format::TF_t::TF_12H ? time_format::TF_t::TF_12H : time_format::TF_t::TF_24H;
    return time_format;
}

bool time_format::HasChanged() {
    return time_format_changed;
}

void time_format::ClearChanged() {
    time_format_changed = false;
}

void time_format::Change(TF_t new_format) {
    TF_t &curr_format = GetTimeFormat();
    if (new_format != curr_format) {
        time_format_changed = true;
        eeprom_set_var(EEVAR_TIME_FORMAT, variant8_ui8((uint8_t)new_format));
    }
    curr_format = new_format;
}

time_format::TF_t time_format::Get() {
    return GetTimeFormat();
}
