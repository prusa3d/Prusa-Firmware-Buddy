#pragma once

#include <cstdint>

namespace modbus {

enum class ServerAddress : uint8_t {
    // Note: sorted alphabetically by server name, not by address

    ac_controller = 219,
    anfc0 = 221,
    anfc1 = 222,
    indx_head = 0x1a + 8,
    invalid = 255,
    mmu = 220,
    tool_offset_sensor = 223,
    xbuddy_extension = 0x1a + 7,
    xl_can = 0x1a + 9,
};

}
