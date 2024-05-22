#pragma once

#include <cstdint>

namespace hal::GPIODriver {

enum class GPIO_Port {
    Port_A = 1,
    Port_B,
    Port_C,
    Port_D,
    Port_E,
    Port_F,
};

enum class GPIO_Pin : uint16_t {
    Pin_0 = 0x0001,
    Pin_1 = 0x0002,
    Pin_2 = 0x0004,
    Pin_3 = 0x0008,
    Pin_4 = 0x0010,
    Pin_5 = 0x0020,
    Pin_6 = 0x0040,
    Pin_7 = 0x0080,
    Pin_8 = 0x0100,
    Pin_9 = 0x0200,
    Pin_10 = 0x0400,
    Pin_11 = 0x0800,
    Pin_12 = 0x1000,
    Pin_13 = 0x2000,
    Pin_14 = 0x4000,
    Pin_15 = 0x8000,
    Pin_All = 0xFFFF,
};

bool Init();

bool ReadPANICSignal();

bool ReadFAULTSignal();

void ResetOverCurrentFault();

} // namespace hal::GPIODriver
