/**
 * @file touch_dependency.hpp
 * @brief api to get touch controller reset
 */
#pragma once

namespace touch {
using reset_clr_fnc_t = void (*)();
void reset_chip(reset_clr_fnc_t reset_clr_fnc);
bool set_registers();
bool is_enabled();
void enable();
void disable();
int get_touch_read_err_total();
}; // namespace touch
