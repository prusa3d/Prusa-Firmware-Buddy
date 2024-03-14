#pragma once
#include "c_fan_ctl.hpp"

class Fans {
    Fans() = default;
    Fans(const Fans &) = default;

public:
    static CFanCtl &print(size_t index);
    static CFanCtl &heat_break(size_t index);
    static void tick();
};
