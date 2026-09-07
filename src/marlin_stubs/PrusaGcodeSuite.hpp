/**
 * @file
 */
#pragma once
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include <option/has_heaters_selftest_gcode.h>
#include <option/has_esp.h>
#include <option/has_toolchanger.h>
#include <option/has_tool_mapping.h>
#include <option/has_side_leds.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_i2c_expander.h>
#include <option/has_chamber_api.h>
#include <option/has_nozzle_cleaner.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_emergency_stop.h>
#include <option/buddy_enable_connect.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_chamber_vents.h>
#include <option/has_spool_join.h>
#include <option/has_indx.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_tool_offset_sensor.h>

#include <gcode/gcode_parser.hpp>

/// the version of the g-code that the printer supports
#define GCODE_LEVEL 2

/**
 * @brief Prusa specific gcode suite
 */
namespace PrusaGcodeSuite {

GcodeSuite::VirtualToolFromCommand get_target_virtual_from_command(const GCodeParser2 &p);
GcodeSuite::VirtualToolFromCommand get_target_virtual_from_command_p(const GCodeParser2 &p);
GcodeSuite::PhysicalToolFromCommand get_target_physical_from_command(const GCodeParser2 &p);
GcodeSuite::PhysicalToolFromCommand get_target_physical_from_command_p(const GCodeParser2 &p);
/** \defgroup G-Codes G-Code Commands
 * @{
 */

#if HAS_NOZZLE_CLEANER() || HAS_NOZZLE_CLEANER_LITE()
void G12(); ///< Nozzle Cleaning
#endif
void G26(); //< first layer calibration
void G162(); //< calibrate Z
void G163(); //< measure length of axis
void G123(); //< Manual move

void M0();

void M104_1(); //< Set hotend temperature with preheat & stealth mode support

void M123(); //< Fan speed reporting

#if HAS_CHAMBER_API()
void M141(); ///< Set chamber temperature
#endif

#if HAS_CHAMBER_FILTRATION_API()
void M147_148(); //< Enable/disable filtration
#endif

void M150();

#if HAS_SIDE_LEDS()
void M151();
#endif

#if HAS_CHAMBER_API()
void M191(); ///< Wait for chamber temperature
#endif

#if HAS_I2C_EXPANDER()
void M262(); //< IO Expander: Configure pin
void M263(); //< IO Expander: Read selected pin
void M264(); //< IO Expander: Set up selected pin
void M265(); //< IO Expander: Toggle selected output pin
void M267(); //< IO Expander: Set register
void M268(); //< IO Expander: Read register
#endif // HAS_I2C_EXPANDER()

void M300(); //< Beep
void M505() = delete; //< set eeprom variable

/// @name MMU G-CODES
/// @{
void M704(); //< Load filament to MMU
void M1704(); //< Load test
void M705(); //< Eject filament from MMU
void M706(); //< Cut filament by MMU
void M707(); //< Read variable from MMU
void M708(); //< Write variable to MMU
void M709(); //< MMU turn on/off/reset
/// @}

#if HAS_TOOL_OFFSET_SENSOR()
void G427(); ///< Tool offset calibration (Z-probe + XY tool offset board calib)
void M1985(); //< INDX tool offsets calibration
#endif

#if HAS_INDX()
void G750(); ///< Move to absolute X,Y position with nozzle cleaner origin offset
#endif

#ifdef PRINT_CHECKING_Q_CMDS
/// @name Print checking commands
///
/// ## Common parameters
///
/// - `Q` - get machine value
///       - query is done during gcode execution (printing)
/// - `P` - check if supplied value matches machine value
///       - This check is done before starting the print from file
///       - This parameter is ignored during print or if supplied via USB CDC
/// @{
void M862_1(); //< Check nozzle diameter
void M862_2(); //< Check model code
void M862_3(); //< Check model name
void M862_4(); //< Check firmware version
void M862_5(); //< Check gcode level
void M862_6(); //< Check gcode level
/// @}
#endif

#if HAS_TOOL_MAPPING()
void M863(); //< tool mapping control
#endif

#if HAS_SPOOL_JOIN()
void M864(); //< spool join control
#endif

void M865(); //< Set up ad-hoc filament

#if HAS_CHAMBER_VENTS()
void M870(); ///< Open or close ventilation intake
#endif

void M591(); //< configure Filament stuck monitoring
#if PRINTER_IS_PRUSA_iX()
void M853(); //< Align z motors over bed pins/end of axis
#endif
#if HAS_MANUAL_BELT_TUNING()
void M961(); //< Manual Belt tuning
#endif

void M997(); //< Update firmware. Prusa STM32 platform specific
void M999();

#if BUDDY_ENABLE_CONNECT()
void M1200(); //< Set ready for printing (Connect-related)
#endif // BUDDY_ENABLE_CONNECT()

void M1600(); //< Menu change filament. Prusa STM32 platform specific
void M1601(); //< Filament stuck detected, Prusa STM32 platform specific

void M1700(); //< Preheat. Prusa STM32 platform specific
void M1701(); //< Autoload. Prusa STM32 platform specific
void M1702(); //< Coldpull. Prusa platform specific
#if HAS_ESP()
void M1703(); //< Wi-fi setup. Prusa platform specific
#endif

#if HAS_AUTO_RETRACT()
void M1705(); //< Autoretract
#endif

void M1978(); //< Fan Selftest
#if HAS_DOOR_SENSOR_CALIBRATION()
void M1980(); //< Door sensor calibration
#endif
#if HAS_SELFTEST()
void M1981(); //< Filament sensors selftest
#endif
#if HAS_INDX()
void M1982(); //< INDX dock calibration
void M1983(); //< INDX nozzle cleaner calibration
void M1984(); //< Manually park a stuck nozzle into a dock
#endif
#if HAS_WASTEBIN_FILL_TRACKING()
void M1986(); //< Empty the INDX nozzle-cleaner wastebin (pause, move aside, reset fill counter)
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
void M1987(); //< Heater selftest
#endif

void M9140(); //< Set normal (non-stealth) mode
void M9141(); //< Get stealth mode status
void M9150(); //< Set stealth mode

void M9200(); //< Re-load IS settings from config store
void M9201(); //< Reset to default motion parameters (accelerations, feedrates, ...)

#if HAS_PRECISE_HOMING_COREXY()
void M9202(); //< Clear precise homing calibration
#endif

void M9933(); //< Cork for tracking when gcode finished executing

#if HAS_CRASH_DETECTION()
void M9944(); ///< Execute G-Code interrupt
#endif

#if EXTRUDERS > 1
void M9934(); ///< Multi filament change
#endif

#if HAS_TOOLCHANGER()
void P0(); //< Tool park
#endif

/** @}*/

} // namespace PrusaGcodeSuite
