// time_tools.cpp

#include "time_tools.hpp"
#include <configuration_store.hpp>

static bool time_format_changed = true;

static time_format::TF_t &GetTimeFormat() {
    static time_format::TF_t time_format = config_store().time_format.get() == time_format::TF_t::TF_12H ? time_format::TF_t::TF_12H : time_format::TF_t::TF_24H;
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
        config_store().time_format.set(new_format);
    }
    curr_format = new_format;
}

time_format::TF_t time_format::Get() {
    return GetTimeFormat();
}
