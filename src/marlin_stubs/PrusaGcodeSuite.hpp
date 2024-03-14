/**
 * @file
 */
#pragma once
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"

#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>

/// the version of the g-code that the printer supports
#define GCODE_LEVEL 2

/**
 * @brief Prusa specific gcode suite
 */
namespace PrusaGcodeSuite {
using M862_6SupportedFeatures = std::array<const char *, 1>;
extern M862_6SupportedFeatures m862_6SupportedFeatures;

void G26(); ///< first layer calibration
void G64(); ///< Measure Z_AXIS height
void G162(); ///< calibrate Z
void G163(); ///< measure length of axis

void M0();
void M50(); ///< selftest

void M123(); ///< Fan speed reporting

void M150();

#if HAS_SIDE_LEDS()
void M151();
#endif

void M300(); ///< Beep
// void M505(); ///< set eeprom variable // deprecated

/// @name MMU G-CODES
/// @{
void M704(); ///< Load filament to MMU
void M1704(); ///< Load test
void M705(); ///< Eject filament from MMU
void M706(); ///< Cut filament by MMU
void M707(); ///< Read variable from MMU
void M708(); ///< Write variable to MMU
void M709(); ///< MMU turn on/off/reset
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
void M862_1(); ///< Check nozzle diameter
void M862_2(); ///< Check model code
void M862_3(); ///< Check model name
void M862_4(); ///< Check firmware version
void M862_5(); ///< Check gcode level
void M862_6(); ///< Check gcode level
/// @}
#endif

#if ENABLED(PRUSA_TOOL_MAPPING)
void M863(); ///< tool mapping control
#endif

#if ENABLED(PRUSA_SPOOL_JOIN)
void M864(); ///< spool join control
#endif

void M591(); ///< configure Filament stuck monitoring

void M930();
void M931();
void M932();
void M997(); ///< Update firmware. Prusa STM32 platform specific
void M999();

void M1587(); ///< Wi-Fi credentials
void M1600(); ///< Menu change filament. Prusa STM32 platform specific
void M1601(); ///< Filament stuck detected, Prusa STM32 platform specific
void M1700(); ///< Preheat. Prusa STM32 platform specific
void M1701(); ///< Autoload. Prusa STM32 platform specific

#if HAS_TOOLCHANGER()
void P0(); ///< Tool park
#endif
} // namespace PrusaGcodeSuite
