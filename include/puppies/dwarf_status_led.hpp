#pragma once

#include <include/dwarf_registers.hpp>
#include <utility_extensions.hpp>

namespace dwarf_shared {

/**
 * @brief Encode and decode dwarf status LED control to be sent over modbus.
 */
struct StatusLed {
    /// Size of raw data in modbus registers
    static constexpr size_t REG_SIZE = (ftrstd::to_underlying(registers::SystemHoldingRegister::status_color_end)
        - ftrstd::to_underlying(registers::SystemHoldingRegister::status_color_start) + 1);
    static_assert(REG_SIZE >= 2, "Needs 2 modbus registers");

    /// Mode of the Dwarf status LED
    enum class Mode : uint8_t {
        dwarf_status = 0, ///< Show dwarf internal status
        solid_color = 1, ///< Show continuous color
        blink = 2, ///< Blink on and off
        pulse = 3, ///< Pulse on and off
        pulse_w = 4, ///< Pulse color and white
        // others - reserved
    };

    uint8_t r; // Red [0 - 0xff]
    uint8_t g; // Green [0 - 0xff]
    uint8_t b; // Blue [0 - 0xff]
    Mode mode; // Show dwarf state, blink, pulse

    /// Default is follow internal status
    StatusLed()
        : r(0)
        , g(0)
        , b(0)
        , mode(Mode::dwarf_status) {}

    /// Construct with given values
    StatusLed(Mode led_mode, uint8_t red, uint8_t green, uint8_t blue)
        : r(red)
        , g(green)
        , b(blue)
        , mode(led_mode) {}

    /// Construct from modbus registers
    StatusLed(std::array<uint16_t, REG_SIZE> modbus_regs) {
        r = modbus_regs[0];
        g = modbus_regs[0] >> 8;
        b = modbus_regs[1];
        mode = static_cast<Mode>(modbus_regs[1] >> 8);
    }

    /**
     * @brief Get modbus register values.
     * @param i register index [0 - 1 (undefined on other values)]
     * @return modbus register value
     */
    uint16_t get_reg_value(size_t i) {
        if (i == 0) {
            return r | (g << 8);
        } else {
            return b | (ftrstd::to_underlying(mode) << 8);
        }
    }

    /**
     * @brief Get modbus register address.
     * @param i register index [0 - 1 (undefined on other values)]
     * @return modbus register address
     */
    static constexpr registers::SystemHoldingRegister get_reg_address(const size_t i) {
        if (i == 0) {
            return registers::SystemHoldingRegister::status_color_start;
        } else {
            return static_cast<registers::SystemHoldingRegister>(ftrstd::to_underlying(registers::SystemHoldingRegister::status_color_start) + 1);
        }
    }
};

} // namespace dwarf_shared
