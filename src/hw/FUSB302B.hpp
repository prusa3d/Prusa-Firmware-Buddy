#pragma once

#include <device/peripherals.h>
#include <device/hal.h>
#include <cstdint>
#include "i2c.hpp"

namespace buddy {
namespace hw {
    class FUSB302B {
    public:
        static void InitChip();
        static bool ReadVBUSState();
        static void ClearVBUSIntFlag();

    private:
        static uint8_t address;
        constexpr static uint8_t WRITE_FLAG = 0x01;

        static void DetectAddress();
    };
} // namespace hw
} // namespace buddy
