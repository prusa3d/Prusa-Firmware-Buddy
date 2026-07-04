/// @file
/// Small shared helpers for the I2C IO expander G-codes (M266/M269).
/// Only usable on builds with HAS_I2C_EXPANDER().
#pragma once

#include "TCA6408A.hpp"
#include <config_store/store_instance.hpp>

namespace io_expander_utils {

/// \returns whether the pin is configured as input in the (EEPROM-backed) Config register
inline bool is_pin_configured_as_input(uint8_t pin) {
    return config_store().io_expander_config_register.get() & (1 << pin);
}

/// Writes the Output register bits selected by \p mask without persisting anything
/// to EEPROM (unlike M264/M267, which store the Output register to the config store
/// on every change). Bits outside \p mask keep their current state (read-modify-write);
/// the final I2C write applies all changed bits at once.
inline bool write_output_volatile(uint8_t value, uint8_t mask) {
    auto &expander = buddy::hw::io_expander2;
    if (mask != 0xFF) {
        uint8_t output;
        if (!expander.read_reg(buddy::hw::TCA6408A::Register::Output, output)) {
            return false;
        }
        value = (output & ~mask) | (value & mask);
        if (value == output) {
            return true;
        }
    }
    return expander.write_reg(buddy::hw::TCA6408A::Register::Output, value);
}

} // namespace io_expander_utils
