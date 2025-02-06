#pragma once

#include <cstdint>

namespace xbuddy_extension_shared {

enum FilamentSensorState {
    uninitialized,
    no_filament,
    has_filament,
    disconnected,
};

static constexpr uint8_t fan_count = 3;

} // namespace xbuddy_extension_shared
