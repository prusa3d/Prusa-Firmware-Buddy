/// @file
/// @brief M1988: INDX nozzle cleaning wizard

#include <option/has_indx.h>

#if HAS_INDX()

    #include "PrusaGcodeSuite.hpp"
    #include <config_store/store_instance.hpp>
    #include <feature/indx_nozzle_cleaning/indx_nozzle_cleaning.hpp>
    #include <gcode/gcode_parser.hpp>
    #include <logging/log.hpp>
    #include <test_result.hpp>
    #include <tool_index.hpp>

LOG_COMPONENT_REF(PRUSA_GCODE);

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1988: Nozzle cleaning wizard
 *
 * Heats each selected tool to the temperature of its loaded filament (230 °C without filament)
 * and cleans its nozzle on the nozzle cleaner. In the manual mode, the user cleans the nozzle
 * hot, then again after cooling down to 180 °C, before the nozzle cleaner cleans it.
 *
 * Requires calibrated docks and nozzle cleaner.
 *
 *#### Usage
 *
 *    M1988 [ P | S ]
 *
 *#### Parameters
 *
 * - `P` - Bit mask of the physical tools to clean (default: all enabled tools)
 * - `S` - Mode
 *   - `0` - (Default) Automatic
 *   - `1` - Manual
 *
 *#### Examples
 *
 *    M1988 P5 S1 ; Clean tools 1 and 3 manually
 */
void PrusaGcodeSuite::M1988() {
    GCodeParser2 parser;
    if (!parser.parse_marlin_command()) {
        return;
    }

    uint32_t enabled_mask = 0;
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        enabled_mask |= uint32_t { 1 } << tool.to_raw();
    }

    const uint32_t tool_mask = parser.option<uint16_t>('P').value_or(enabled_mask) & enabled_mask;
    const auto mode_opt = parser.option<uint8_t>('S', uint8_t { 0 }, uint8_t { 1 });

    if (parser.has_option('S') && !mode_opt.has_value()) {
        log_warning(PRUSA_GCODE, "M1988: invalid mode");
        return;
    }
    if (tool_mask == 0) {
        log_warning(PRUSA_GCODE, "M1988: no enabled tool selected");
        return;
    }
    if (config_store().indx_dock_calibrated_mask.get().none()
        || config_store().selftest_result_nozzle_cleaner_calibration.get() != TestResult::passed) {
        SERIAL_ERROR_MSG("M1988: docks and nozzle cleaner must be calibrated");
        return;
    }

    indx_nozzle_cleaning::run(tool_mask, static_cast<indx_nozzle_cleaning::Mode>(mode_opt.value_or(0)));
}

/** @}*/

#endif
