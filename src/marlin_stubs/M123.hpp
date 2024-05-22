/**
 * @file M123.hpp
 * @brief fans speed reporting as defined in https://marlinfw.org/docs/gcode/M123.html
 */

#pragma once
#include <cstdint>

namespace M123 {
extern uint32_t fan_auto_report_delay;
void print_fan_speed();
} // namespace M123
