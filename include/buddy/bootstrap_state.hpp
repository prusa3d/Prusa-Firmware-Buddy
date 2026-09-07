/// @file
#pragma once

#include <cstdint>
#include <option/bootloader_update.h>
#include <option/has_ac_controller.h>
#include <option/has_dwarf.h>
#include <option/has_esp.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_puppies.h>
#include <option/has_puppy_modularbed.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_indx_head.h>
#include <option/has_xl_can.h>
#include <option/resources.h>

namespace buddy {

enum class BootstrapStage : uint8_t {
    initial = 0,
#if RESOURCES() || BOOTLOADER_UPDATE()
    looking_for_bbf,
#endif
#if RESOURCES()
    preparing_bootstrap,
    copying_files,
#endif
#if BOOTLOADER_UPDATE()
    preparing_update,
    updating,
#endif
#if HAS_ESP()
    flashing_esp,
    reflashing_esp,
#endif
#if HAS_XBUDDY_EXTENSION()
    looking_for_xbuddy_extension,
    verifying_xbuddy_extension,
    flashing_xbuddy_extension,
#endif
#if HAS_PUPPIES()
    waking_up_puppies,
    looking_for_puppies,
    verifying_puppies,
    #if HAS_DWARF()
    flashing_dwarf,
    verifying_dwarf,
    #endif
    #if HAS_PUPPY_MODULARBED()
    flashing_modular_bed,
    verifying_modular_bed,
    #endif
    #if HAS_AC_CONTROLLER()
    ac_controller_unknown,
    ac_controller_verify,
    ac_controller_flash,
    ac_controller_ready,
    #endif
    #if HAS_TOOL_OFFSET_SENSOR()
    tool_offset_sensor_unknown,
    tool_offset_sensor_verify,
    tool_offset_sensor_flash,
    tool_offset_sensor_ready,
    #endif
    #if HAS_INDX_HEAD()
    flashing_indx_head,
    verifying_indx_head,
    #endif
    #if HAS_XL_CAN()
    looking_for_xl_can,
    flashing_xl_can,
    verifying_xl_can,
    #endif
#endif
};

struct BootstrapState {
    BootstrapStage stage;
    uint8_t percent;

    constexpr auto operator<=>(const BootstrapState &) const = default;
};

BootstrapState bootstrap_state_get();

void bootstrap_state_set(BootstrapState);

inline void bootstrap_state_set(uint8_t percent, BootstrapStage stage) {
    return bootstrap_state_set({ stage, percent });
}

} // namespace buddy
