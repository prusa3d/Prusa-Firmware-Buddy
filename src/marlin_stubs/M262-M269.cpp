#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"

#include "Marlin/src/Marlin.h"
#include "Marlin/src/module/planner.h"

#include <marlin_server.hpp>
#include <marlin_stubs/io_expander_utils.hpp>
#include <marlin_stubs/skippable_gcode.hpp>
#include <marlin_stubs/wait_deadline.hpp>

#include "timing.h"

// THESE G-CODES ARE AVAILABLE ONLY ON XBUDDY AND XLBUDDY BOARDS

static constexpr const char txt_failed_comm[] = " Error: Failed i2c communication\n";
static constexpr const char txt_not_initialized[] = " Error: IO Expander not initialized properly\n";

/// Polling period of the M269 wait. Reading the expander blocks the marlin
/// thread for ~5 ms on I2C, so we do not want to poll it on every idle() pass.
static constexpr uint32_t wait_poll_period_ms = 100;

static std::optional<uint32_t> parse_timeout_ms(const GCodeParser2 &p) {
    if (const auto timeout_s = p.option<uint32_t>('T', uint32_t { 1 }, uint32_t { 24 * 60 * 60 })) {
        return *timeout_s * 1000;
    }
    return std::nullopt;
}

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
 *### M262: Configure IO Expander pin <a href="https://reprap.org/wiki/G-code#M262:_Configure_IO_Expander_pin">M262: Configure IO Expander pin</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S
 *
 *#### Usage
 *
 *    M262 [ P | B ]
 *
 *#### Parameters
 *
 * - `P` - Single pin <0;7> to be configured.
 * - `B` - Pin mode
 *   - `0` - As output pin
 *   - `1` - As input pin
 *
 *#### Examples
 *
 *    M262 P0 B0 ; Set pin0 as Output pin (0)
 *
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
 *### M263: Read IO Expander pin <a href="https://reprap.org/wiki/G-code#M263:_Read_IO_Expander_pin">M263: Read IO Expander pin</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S
 *
 *#### Usage
 *
 *    M263 [ P ]
 *
 *#### Parameters
 *
 * - `P` - Single pin to read <0;7>.
 *
 *#### Examples
 *
 *    M263 P6 ; read only from pin6, received value will be HIGH (binary 0010 0000 => dec 32) or LOW (binary 0000 0000 => dec 0)
 *    M263    ; read whole Input register (byte)
 *
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
 *### M264: Write IO Expander pin <a href="https://reprap.org/wiki/G-code#M264:_Write_IO_Expander_pin">M264: Write IO Expander pin</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S
 *
 *#### Usage
 *
 *    M264 [ P | B ]
 *
 *
 *#### Parameters
 *
 * - `P` - Select single pin to write to <0;7>.
 * - `B` - Set pin
 *   - `1` - Set HIGH
 *   - `0` - Set LOW
 *
 *#### Examples
 *
 *    M264 P0 B1 ; Set output pin0 to HIGH (1)
 *    M264 P7 B0 ; Set output pin7 to LOW (0)
 *
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
 *### M265: Toggle IO Expander output pin <a href="https://reprap.org/wiki/G-code#M265:_Toggle_IO_Expander_output_pin">M265: Toggle IO Expander output pin</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S
 *
 *#### Usage
 *
 *    M265 [ P ]
 *
 *
 * This G-Code doesn't check if selected pin is configured as Output pin.
 *
 *#### Parameters
 *
 *  - `P` - Select single pin to flip <0;7>.
 *
 *#### Examples
 *
 *    M264 P0 ; Flip pin0
 *
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
 *### M266: Write IO Expander output without EEPROM persistence
 *
 * Sets an IO expander output pin (or the whole Output register), like M264/M267,
 * but does NOT store the value to the config store. Intended for frequent
 * signalling (e.g. once per toolchange or a per-layer timelapse trigger)
 * where M264/M267 would wear the EEPROM out.
 *
 *#### Usage
 *
 *    M266 [ P | B | R | M ]
 *
 *#### Parameters
 *
 * - `P` - Single pin <0;7> to write to
 * - `B` - Value to write into the Output register; with `P` nonzero = 1, with `R` the whole register byte <0;255>
 * - `R` - Register variant; only `R1` (Output register) is allowed
 * - `M` - Bit mask <0;255> for the `R` variant: only the selected bits are changed,
 *         the others keep their state (read-modify-write). Default 255 = whole register.
 *         Use it to leave pins owned by other functions (e.g. a timelapse trigger) untouched.
 *
 * Note: on the GPIO Hackerboard, pins 0-3 are open-drain power outputs - writing `1`
 * switches the pin to GND, so the connector level is inverted to the register value.
 * Pins 4-7 follow the register value (`1` = ~3.3V).
 *
 *#### Examples
 *
 *    M266 P0 B1      ; activate output pin0 (Hackerboard: switched to GND), no EEPROM write
 *    M266 R1 B6 M14  ; set pins 1-3 to 0b011, pins 0 and 4-7 untouched
 */
void PrusaGcodeSuite::M266() {
    static constexpr const char *gcode = "M266";
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }
    if (!buddy::hw::io_expander2.is_initialized()) {
        SERIAL_ECHOPAIR(gcode, txt_not_initialized);
        return;
    }

    const auto value = p.option<uint8_t>('B');
    if (!value.has_value()) {
        SERIAL_ECHOLIST(gcode, " Bad request: B parameter is missing\n");
        return;
    }

    if (const auto reg = p.option<uint8_t>('R')) {
        if (*reg != static_cast<uint8_t>(buddy::hw::TCA6408A::Register::Output)) {
            SERIAL_ECHOLIST(gcode, " Bad request: only the Output register (R1) can be written\n");
            return;
        }
        const uint8_t mask = p.option<uint8_t>('M').value_or(0xFF);
        if (!io_expander_utils::write_output_volatile(*value, mask)) {
            SERIAL_ECHOPAIR(gcode, txt_failed_comm);
        }
        return;
    }

    const auto pin = p.option<uint8_t>('P', uint8_t { 0 }, uint8_t { buddy::hw::TCA6408A::pin_count - 1 });
    if (!pin.has_value()) {
        SERIAL_ECHOLIST(gcode, " Bad request: P parameter is missing or out of range <0;7>\n");
        return;
    }
    if (!io_expander_utils::write_output_volatile(*value != 0 ? 0xFF : 0, 1 << *pin)) {
        SERIAL_ECHOPAIR(gcode, txt_failed_comm);
    }
}

/**
 *### M267: Write IO Expander register <a href="https://reprap.org/wiki/G-code#M267:_Write_IO_Expander_register">M267: Write IO Expander register</a>
 *
 * This overwrites whole byte. Configuration and Output registers are saved into persistent memory.
 *
 * Only MK3.5/S, MK3.9/S, MK4/S
 *
 *#### Usage
 *
 *    M [ R | B ]
 *
 *#### Parameters
 *
 *  - `R` - Register
 *      - Output = 1,
 *      - Polarity = 2,
 *      - Config = 3
 *
 *  - `B` - [value] to be set
 *
 *#### Examples
 *
 *    M267 R3 B255 ; Set up Config register to the value 255dec (1111 1111b)
 *
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
 *### M268: Read IO expander register <a href="https://reprap.org/wiki/G-code#M268:_Read_IO_expander_register">M268: Read IO expander register</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S
 *
 *#### Usage
 *
 *    M268 [ R ]
 *

 *#### Parameters
 *
 * - `R` - Register
 *   - `0` - Input register
 *   - `1` - Output register
 *   - `2` - Polarity register
 *   - `3` - Config register
 *
 *#### Examples
 *
 *    M268 R3 ; Read Config register
 *
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

/**
 *### M269: Wait for IO Expander pin state
 *
 * Blocks until the selected IO expander input pin reads the requested state.
 * The print head stays where it is, temperatures are kept, the print is NOT paused.
 * Designed as the handshake "external device has finished" (see doc/gpio_hackerboard.md).
 *
 * Like M263, this G-code only reads the pin - it does not reconfigure its
 * direction. Configure the pin as input beforehand (M262); on the GPIO
 * Hackerboard only pins 4-7 are capable of acting as inputs.
 *
 * On timeout the print gets paused (M601-like) and the G-code finishes,
 * so the situation is visible and recoverable from the display.
 * The wait can also be ended by aborting the print.
 *
 *#### Usage
 *
 *    M269 [ P | S | T ]
 *
 *#### Parameters
 *
 * - `P` - Pin <0;7> to watch
 * - `S` - State to wait for: `1` = HIGH (default), `0` = LOW
 * - `T` - Timeout in seconds; without it the wait is unlimited
 *
 *#### Examples
 *
 *    M269 P6 S0 T300 ; wait up to 5 minutes for pin6 to go LOW
 */
void PrusaGcodeSuite::M269() {
    static constexpr const char *gcode = "M269";
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }
    if (!buddy::hw::io_expander2.is_initialized()) {
        SERIAL_ECHOPAIR(gcode, txt_not_initialized);
        return;
    }

    const auto pin = p.option<uint8_t>('P', uint8_t { 0 }, uint8_t { buddy::hw::TCA6408A::pin_count - 1 });
    if (!pin.has_value()) {
        SERIAL_ECHOLIST(gcode, " Bad request: P parameter is missing or out of range <0;7>\n");
        return;
    }

    const bool target_state = p.option<bool>('S').value_or(true);
    const WaitDeadline deadline(ticks_ms(), parse_timeout_ms(p));

    const uint8_t mask = 1 << *pin;
    SkippableGCode::Guard skippable_operation;
    uint32_t last_poll_ms = ticks_ms() - wait_poll_period_ms; // poll right away

    while (true) {
        if (planner.draining() || skippable_operation.is_skip_requested()) {
            // Print abort or skip -> stop waiting
            return;
        }

        const uint32_t now = ticks_ms();
        if (deadline.expired(now)) {
            SERIAL_ERROR_START();
            SERIAL_ECHOPAIR(gcode, " timeout while waiting for the pin, pausing the print\n");
            marlin_server::print_pause();
            return;
        }

        if (ticks_diff(now, last_poll_ms) >= static_cast<int32_t>(wait_poll_period_ms)) {
            last_poll_ms = now;
            if (const auto value = buddy::hw::io_expander2.read(mask)) {
                if ((*value != 0) == target_state) {
                    return; // pin reached the requested state
                }
            }
            // I2C errors are not fatal here - just keep polling (covered by T timeout)
        }

        idle(true);
    }
}

/** @}*/
