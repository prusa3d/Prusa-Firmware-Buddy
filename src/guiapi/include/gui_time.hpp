#pragma once
/**
 * @file gui_time.hpp
 * @author Radek Vana
 * @brief wrapped access to tick functions
 * @date 2021-04-21
 */

#include <cstdint>

namespace gui {
void TickLoop(); // call this function in loop
void StartLoop(); // call this function in the beginning of the GUI loop
void EndLoop(); // call this function in the beginning of the GUI loop
uint32_t GetLoopCounter(); // current loop counter (is incremented with each loop by one)
uint32_t GetTick(); // current loop tick value, every call in current loop returns same value
uint32_t GetTick_ForceActualization(); // needed during gui start
}; // namespace gui
