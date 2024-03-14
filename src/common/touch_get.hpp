/**
 * @file touch_get.hpp
 * @brief api to get touched point in GUI
 */
#pragma once
#include "guitypes.hpp"
#include <optional>

namespace touch {
std::optional<point_ui16_t> Get();
bool download_as_new();
bool download(const char *fn);
bool upload(const char *fn);
void poll();
bool is_hw_broken();
bool does_read_work();
bool is_enabled();
void disable();
}; // namespace touch
