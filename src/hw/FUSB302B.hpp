#pragma once

#include <device/peripherals.h>
#include <device/hal.h>
#include <cstdint>
#include "i2c.h"

namespace buddy {
namespace hw {
    class FUSB302B {
    public:
#if (BOARD_IS_XBUDDY)
        static constexpr uint8_t FUSB302B_SLAVE_ADDRESS = 0x23;
#elif (BOARD_IS_XLBUDDY)
        static constexpr uint8_t FUSB302B_SLAVE_ADDRESS = 0x22;
#else
    #error Board not support USBC driver
#endif

        static constexpr uint8_t WRITE_ADDRESS = FUSB302B_SLAVE_ADDRESS << 1 | 0x1;
        static constexpr uint8_t READ_ADDRESS = FUSB302B_SLAVE_ADDRESS << 1 | 0x0;

        static void ResetChip() {
            uint8_t _sw_reset[2] = { 0x0C, 0x03 };
            I2C_Transmit(&I2C_HANDLE_FOR(usbc), WRITE_ADDRESS, _sw_reset, 2, 100);
        };
        static void InitChip() {
            uint8_t _enable_power[2] = { 0x0B, 0x0F };     // wake up circuit
            uint8_t _enable_interupts[2] = { 0x06, 0x05 }; // enable all interrupts
            I2C_Transmit(&I2C_HANDLE_FOR(usbc), WRITE_ADDRESS, _enable_power, 2, 100);
            I2C_Transmit(&I2C_HANDLE_FOR(usbc), WRITE_ADDRESS, _enable_interupts, 2, 100);
        }

        static uint8_t ReadSTATUS0Reg() {
            uint8_t _data_form_chip = 0x00;
            uint8_t _data_to_chip[1] = { 0x40 };
            I2C_Transmit(&I2C_HANDLE_FOR(usbc), WRITE_ADDRESS, _data_to_chip, 1, 100);
            I2C_Receive(&I2C_HANDLE_FOR(usbc), READ_ADDRESS, &_data_form_chip, 1, 100);
            return _data_form_chip;
        }
    };
}
}
