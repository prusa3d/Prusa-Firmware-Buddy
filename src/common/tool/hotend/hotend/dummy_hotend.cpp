/// @file
#include "dummy_hotend.hpp"

namespace {
/// NoTool has no real hotend; a zero config makes max_nozzle_temp() report 0.
/// Static storage so Hotend's base_config_ reference stays valid for the DummyHotend's lifetime.
constexpr Hotend::Config dummy_config {
    .min_nozzle_temp = 0,
    .max_nozzle_temp = 0,
};
} // namespace

DummyHotend::DummyHotend()
    : Hotend(dummy_config) {
    nozzle_temp_reached_ = true;
}
