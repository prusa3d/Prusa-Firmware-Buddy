//display.hpp
#pragma once

#include "display.h"
#include "st7789v.h"

constexpr const display_t &Disp() {
    return st7789v_display;
}
