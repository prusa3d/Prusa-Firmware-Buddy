#pragma once

#include <cstdint>

namespace modbus {

enum class ServerAddress : uint8_t {
    // Note: sorted alphabetically by server name, not by address

    ac_controller = 0xdb,
    anfc0 = 0xdd,
    anfc1 = 0xde,
    indx_head = 0x22, // 0x1a + 8
    invalid = 0xff,
    mmu = 0xdc,
    tool_offset_sensor = 0xdf,
    xbuddy_extension = 0x21, // 0x1a + 7
    xl_can = 0x23, // 0x1a + 9
};

}
