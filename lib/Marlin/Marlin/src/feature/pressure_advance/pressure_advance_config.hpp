#pragma once

#include <cstdint>

namespace pressure_advance {

struct Config {
    float pressure_advance = 0.f;
    float smooth_time;
};

// pressure advance initialization
void init();

// configure e axis
void set_axis_e_config(const Config &config);
const Config &get_axis_e_config();

}
