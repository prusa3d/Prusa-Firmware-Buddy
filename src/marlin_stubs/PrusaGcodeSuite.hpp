/**
 * @file
 */
#pragma once
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"

#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>
#include <option/has_belt_tuning.h>
#include <option/has_i2c_expander.h>
#include <option/has_chamber_api.h>
#include <option/has_nozzle_cleaner.h>
#include <option/buddy_enable_connect.h>

/// the version of the g-code that the printer supports
#define GCODE_LEVEL 2

/**
 * @brief Prusa specific gcode suite
 */
namespace PrusaGcodeSuite {

/// \returns FilamentType read from the gcode parser under \p parameter.
/// The expected format is S"Filament name" (where S = \p parameter)
/// If \param string_begin_ptr is provided, it is set to the begining of the filament string name
FilamentType get_filament_type_from_command(char parameter, const char **string_begin_ptr = nullptr);

/** \defgroup G-Codes G-Code Commands
 * @{
 */

#if HAS_NOZZLE_CLEANER()
void G12(); ///< Nozzle Cleaning
#endif

void G26(); ///< first layer calibration
void G64(); ///< Measure Z_AXIS height
void G162(); ///< calibrate Z
void G163(); ///< measure length of axis

void M0();

void M104_1(); //< Set hotend temperature with preheat & stealth mode support

void M123(); //< Fan speed reporting

#if HAS_CHAMBER_API()
void M141(); ///< Set chamber temperature
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
// void M505(); //< set eeprom variable // deprecated

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

#if ENABLED(PRUSA_TOOL_MAPPING)
void M863(); //< tool mapping control
#endif

#if ENABLED(PRUSA_SPOOL_JOIN)
void M864(); //< spool join control
#endif

void M591(); //< configure Filament stuck monitoring

#if HAS_BELT_TUNING()
void M960(); //< Belt tuning
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
void M1703(); //< Wi-fi setup. Prusa platform specific

void M9140(); //< Set normal (non-stealth) mode
void M9150(); //< Set stealth mode

void M9200(); //< Re-load IS settings from config store
void M9201(); //< Reset to default motion parameters (accelerations, feedrates, ...)

#if HAS_TOOLCHANGER()
void P0(); //< Tool park
#endif

/** @}*/

} // namespace PrusaGcodeSuite
