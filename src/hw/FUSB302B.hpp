#pragma once

#include <device/peripherals.h>
#include <device/hal.h>
#include <cstdint>
#include "i2c.hpp"

namespace buddy {
namespace hw {
    class FUSB302B {
    public:
        enum class VBUS_state {
            not_detected = 0x00,
            detected = 0x80
        };

        static void InitChip();
        static VBUS_state ReadSTATUS0Reg();

    private:
        static uint8_t address;
        constexpr static uint8_t WRITE_FLAG = 0x01;

        static void DetectAddress();
    };
} // namespace hw
} // namespace buddy
