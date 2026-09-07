#include <Marlin.h>
#include <gcode/gcode.h>
#include <feature/contactless_offset/contactless_offset.hpp>
#include <module/motion.h>
#include <module/planner.h>
#include <feature/pressure_advance/pressure_advance_config.hpp>
#include <utils/variant_utils.hpp>
#include <option/has_toolchanger.h>
#include <option/tool_offset_sensor_geometry.h>
#include <module/tool_change.h>
#include <module/prusa/toolchanger.h>
#include <feature/tool_offset_calibration/tool_offset_calibration.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 * ### G426: Measure tool offset using contactless sensor
 *
 * Uses the induction sensor to measure the current tool's offset relative to a
 * reference position.
 *
 * #### Usage
 *
 *     G426 [F<speed>] [R<speed2>] [Z<height>] [D<diameter>] [X<pos_x>] [Y<pos_y>]
 *
 * #### Parameters
 *
 * - `F` - First speed (v1) in mm/s (default: 7)
 * - `R` - Second speed (v2) in mm/s (default: 15)
 * - `Z` - Sensing height in mm above the sensor (default: from config)
 * - `X` - position of the sensor in X axis (default: from config), single-coil printers only
 * - `Y` - position of the sensor in Y axis (default: from config), single-coil printers only
 * - `D` - Scan diameter in mm (default: from config), the same distance is used for both X and Y directions
 * - `I` - Scan length in X axis in mm (default: from config) - overwrites `D` if set
 * - `J` - Scan length in Y axis in mm (default: from config) - overwrites `D` if set
 */
void GcodeSuite::G426() {
    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);
    SERIAL_ECHOLN("G426 contactless offset measurement");

    auto config = tool_offset::get_default_probing_config();
#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
    tool_offset_calibration::apply_stored_sensor_position(config);
#endif

    config.sensing_speed_slow = parser.floatval('F', config.sensing_speed_slow);
    config.sensing_speed_fast = parser.floatval('R', config.sensing_speed_fast);
    config.sensing_z = parser.floatval('Z', config.sensing_z);
    config.coil_x.sensing_distance = parser.floatval('D', config.coil_x.sensing_distance);
    config.coil_y.sensing_distance = parser.floatval('D', config.coil_y.sensing_distance);
    config.coil_x.sensing_distance = parser.floatval('I', config.coil_x.sensing_distance);
    config.coil_y.sensing_distance = parser.floatval('J', config.coil_y.sensing_distance);
#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
    tool_offset::set_single_coil_position(config,
        { parser.floatval('X', config.coil_x.position.x), parser.floatval('Y', config.coil_x.position.y) });
#endif

    const auto selected_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!selected_tool.has_value()) {
        SERIAL_ECHOLNPAIR("G426 failed: no tool selected");
        return;
    }

    if (tool_offset_calibration::calibrate_xy_offset(selected_tool.value(), config)) {
        SERIAL_ECHOLN("G426 completed successfully");
    } else {
        SERIAL_ECHOLN("G426 failed");
    }

    SERIAL_ECHOPGM("X offset: ");
    SERIAL_ECHO_F(hotend_offset[selected_tool.value()].x, 4);
    SERIAL_ECHOLNPGM(" mm");
    SERIAL_ECHOPGM("Y offset: ");
    SERIAL_ECHO_F(hotend_offset[selected_tool.value()].y, 4);
    SERIAL_ECHOLNPGM(" mm");
    SERIAL_ECHOPGM("Z offset: ");
    SERIAL_ECHO_F(hotend_offset[selected_tool.value()].z, 4);
    SERIAL_ECHOLNPGM(" mm");
}

/** @}*/
