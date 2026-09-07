/// @file
#pragma once

#include <cstdint>

namespace gui {

/// Call this function once in GUI loop.
void TickLoop();

/// Current loop tick value, every call in current loop returns same value.
/// Use this instead of naive ticks_ms() to ensure consistent timing in GUI
/// and prevent flickering.
uint32_t GetTick();

} // namespace gui
