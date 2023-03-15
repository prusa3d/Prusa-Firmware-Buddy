#pragma once
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"

#include <option/has_toolchanger.h>
// the version of the g-code that the printer supports
#define GCODE_LEVEL 1

namespace PrusaGcodeSuite {
void G26();  /// first layer calibration
void G64();  /// Measure Z_AXIS height
void G162(); /// handler-specific configuration
void G163(); /// measure length of axis

void M50(); /// selftest

void M300(); /// beep
void M505(); /// set eeprom variable
void M650();

// MMU G-CODES
void M704(); /// Load filament to MMU
void M705(); /// Eject filament from MMU
void M706(); /// Cut filament by MMU
void M707(); /// Read variable from MMU
void M708(); /// Write variable to MMU
void M709(); /// MMU turn on/off/reset

#ifdef PRINT_CHECKING_Q_CMDS
void M862_1(); // Check nozzle diameter
void M862_2(); // Check model code
void M862_3(); // Check model name
void M862_4(); // Check firmware version
void M862_5(); // Check gcode level
#endif

void M930();
void M931();
void M932();
void M997(); /// M997 Update firmware. Prusa STM32 platform specific
void M999(); /// M999 reset MCU. Prusa STM32 platform specific

void M1587(); /// Wi-Fi credentials
void M1600(); /// Menu change filament. Prusa STM32 platform specific
void M1700(); /// Preheat. Prusa STM32 platform specific
void M1701(); /// Autoload. Prusa STM32 platform specific
void M150();

#if HAS_TOOLCHANGER()
void P0(); /// Tool park
#endif
}
