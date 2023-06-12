/**
 * @file no_touch.cpp
 * @brief dummy methods for boards without touch
 */
#include "touch_dependency.hpp"
#include "touch_get.hpp"
#include "guitypes.hpp"
#include <optional>

std::optional<point_ui16_t> touch::Get() {
    return std::nullopt;
}
bool touch::is_hw_broken() {
    return true;
}

bool touch::does_read_work() {
    return false;
}
