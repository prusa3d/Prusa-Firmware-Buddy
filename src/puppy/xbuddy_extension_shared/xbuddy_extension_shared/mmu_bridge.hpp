/// @file
#pragma once

#include <cstdint>
#include <tuple>

namespace xbuddy_extension_shared::mmu_bridge {

constexpr uint8_t modbusUnitNr = 220;

// Note: ordering of registers is important, do not change arbitrarily as the modbus register block mapping may break
constexpr uint8_t buttonRegisterAddress = 252;
constexpr uint8_t commandInProgressRegisterAddress = 253;
constexpr uint8_t commandStatusRegisterAddress = 254;
constexpr uint8_t commandProgressOrErrorCodeRegisterAddress = 255;

inline constexpr uint16_t pack_command(uint8_t command, uint8_t param) {
    return (((uint16_t)param) << 8) | command;
}

/// @returns pair [command, param]
inline constexpr std::tuple<uint8_t, uint8_t> unpack_command(uint16_t cmdpacked) {
    return { cmdpacked & 0xff, cmdpacked >> 8 };
}

} // namespace xbuddy_extension_shared::mmu_bridge
