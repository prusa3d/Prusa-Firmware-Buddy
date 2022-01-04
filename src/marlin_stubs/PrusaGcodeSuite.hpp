#pragma once

namespace PrusaGcodeSuite {
void G26();  /// first layer calibration
void G162(); /// handler-specific configuration

void M300(); /// beep
void M999(); /// M999 reset MCU. Prusa STM32 platform specific

void M1400(); /// Preheat. Prusa STM32 platform specific

void M505(); /// set eeprom variable

void M997(); /// M997 Update firmware. Prusa STM32 platform specific
}
