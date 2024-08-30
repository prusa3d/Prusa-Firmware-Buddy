#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"

// THESE G-CODES ARE AVAILABLE ONLY ON XBUDDY AND XLBUDDY BOARDS

static constexpr const char txt_failed_comm[] = " Error: Failed i2c communication\n";

static std::optional<uint8_t> check_param(const char param, const char *gcode, uint16_t num_limit) {
    if (!parser.seen(param)) {
        SERIAL_ECHOLIST(gcode, " Bad request:", param, "<base10> parameter is missing\n");
        return std::nullopt;
    }
    uint16_t num = parser.ushortval(param, 0); // ushort for check of B <0;255>
    if (num >= num_limit) {
        SERIAL_ECHOLIST(gcode, " Bad request", param, "<base10> parameter is out of range <0", num_limit - 1, ">\n");
        return std::nullopt;
    }
    return num;
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *  M262: Configure pin on IO Expander.
 *
 *  ## Parameters
 *
 * - `P` - Single pin <0;7> to be configured.
 * - `B` - Pin mode (0 == Output pin, 1 == Input pin)
 *
 *  ## Examples
 *
 * - `M262 P0 B0` - Set pin0 as Output pin (0)
 */
void PrusaGcodeSuite::M262() {
    static constexpr const char *gcode = "M262";
    const auto pin_num = check_param('P', gcode, buddy::hw::TCA6408A::pin_count);
    const auto val = check_param('B', gcode, 256);

    if (!pin_num.has_value() || !val.has_value()) {
        return;
    }

    if (!buddy::hw::io_expander2.update_register(buddy::hw::TCA6408A::Register::Config, val.value() ? 0xFF : 0, 0x1 << pin_num.value())) {
        SERIAL_ECHOPAIR(gcode, txt_failed_comm);
    }
    return;
}

/**
 *  M263: Read from IO Expander's pin.
 *
 *  ## Parameters
 *
 * - `P` - Single pin to read from <0;7>.
 *
 *  ## Examples
 *
 * - `M263 P6` - read only from pin6, received value will be HIGH (binary 0010 0000 => dec 32) or LOW (binary 0000 0000 => dec 0)
 * - `M263` - read whole Input register (byte)
 */
void PrusaGcodeSuite::M263() {
    static constexpr const char *gcode = "M263";
    uint8_t pin_mask = 0xFF; // Read whole register by default

    const bool seen_P = parser.seen('P');
    if (seen_P) {
        if (const auto pin_num = check_param('P', gcode, buddy::hw::TCA6408A::pin_count)) {
            pin_mask = 0x1 << pin_num.value();
        } else {
            return; // out_of_range returns but not_seen does not
        }
    }

    const auto value = buddy::hw::io_expander2.read(pin_mask);
    if (!value.has_value()) {
        SERIAL_ECHOPAIR(gcode, txt_failed_comm);
        return;
    }

    // Print out on Serial port
    if (seen_P) {
        SERIAL_ECHOPAIR("IO Expander Pin ", value.value() ? "HIGH" : "LOW");
    } else {
        SERIAL_ECHOPAIR("IO Expander Input Register: ", value.value());
    }
    SERIAL_EOL();

    // PLACEHOLDER: Add your code here
}

/**
 *  M264: Write to IO Expander's pin.
 *
 *  ## Parameters
 *
 * - `P` - Select single pin to write to <0;7>.
 * - `B` - [value] to write (0 or 1)
 *
 *  ## Examples
 *
 * - `M264 P0 B1` - Set output pin0 to HIGH (1)
 * - `M264 P7 B0` - Set output pin7 to LOW (0)
 */
void PrusaGcodeSuite::M264() {
    static constexpr const char *gcode = "M264";
    const auto pin_num = check_param('P', gcode, buddy::hw::TCA6408A::pin_count);
    const auto val = check_param('B', gcode, 256);
    if (!pin_num.has_value() || !val.has_value()) {
        return;
    }

    if (buddy::hw::io_expander2.write(val.value() ? 0xFF : 0, 0x1 << pin_num.value())) {
        return;
    }
    SERIAL_ECHOPAIR(gcode, txt_failed_comm);
}

/**
 *  M265: Toggle IO Expander's output pin. This G-Code doesn't check if selected pin is configured as Output pin.
 *
 *  ## Parameters
 *
 *  - `P` - Select single pin to flip <0;7>.
 *
 *  ## Examples
 *
 * - `M264 P0` - Flip pin0
 */
void PrusaGcodeSuite::M265() {
    static constexpr const char *gcode = "M265";
    const auto pin_num = check_param('P', gcode, buddy::hw::TCA6408A::pin_count);
    if (!pin_num.has_value()) {
        return;
    }

    if (buddy::hw::io_expander2.toggle(pin_num.value())) {
        return;
    }
    SERIAL_ECHOPAIR(gcode, txt_failed_comm);
}

/**
 *  M267: Write to IO Expander's register. This overwrites whole byte. Configuration and Output registers are saved into persistent memory.

 *  ## Parameters
 *
 *  - `R` - Register
 *      - Output = 1,
 *      - Polarity = 2,
 *      - Config = 3
 *
 *  - `B` - [value] to be set
 *
 *  ## Examples
 *
 * - `M267 R3 B255` - Set up Config register to the value 255dec (1111 1111b)
 */
void PrusaGcodeSuite::M267() {
    static constexpr const char *gcode = "M267";
    const auto reg = check_param('R', gcode, 4);
    const auto val = check_param('B', gcode, 256);
    if (!reg.has_value() || !val.has_value()) {
        return;
    }
    if (reg.value() == 0) {
        SERIAL_ECHOPAIR(gcode, " Aborted. Input register is read only.\n");
        return;
    }

    if (buddy::hw::io_expander2.update_register(buddy::hw::TCA6408A::Register(reg.value()), val.value())) {
        return;
    }
    SERIAL_ECHOPAIR(gcode, txt_failed_comm);
}

/**
 *  M268: Reads IO expander's register. This reads whole byte.
 *
 *  ## Parameters
 *
 *  - `R` - Register
 *      - Input = 0,
 *      - Output = 1,
 *      - Polarity = 2,
 *      - Config = 3
 *
 *  ## Examples
 *
 * - `M268 R3` - Reads Config register
 */
void PrusaGcodeSuite::M268() {
    static constexpr const char *gcode = "M268";
    const auto reg = check_param('R', gcode, 4);
    if (!reg.has_value()) {
        return;
    }

    uint8_t value;
    if (buddy::hw::io_expander2.read_reg(buddy::hw::TCA6408A::Register(reg.value()), value)) {
        SERIAL_ECHOLN(value);
        return;
    }
    SERIAL_ECHOPAIR(gcode, txt_failed_comm);
}

/** @}*/
