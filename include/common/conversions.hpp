#pragma once

namespace {

constexpr uint8_t val_mapping(const bool rounding_floor, const uint8_t val, const uint8_t max_val, const uint8_t new_max_val) {
    if (rounding_floor) {
        return uint8_t(static_cast<uint32_t>(val) * new_max_val / max_val);
    } else {
        return uint8_t((static_cast<uint32_t>(val) * new_max_val + max_val - 1) / max_val);
    }
}

} // namespace
