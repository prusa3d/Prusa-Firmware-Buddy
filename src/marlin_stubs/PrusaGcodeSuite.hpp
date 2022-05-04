#pragma once

namespace PrusaGcodeSuite {
void G26();  /// first layer calibration
void G162(); /// handler-specific configuration

void M300(); /// beep
void M505(); /// set eeprom variable
void M650();

//MMU G-CODES
void M704(); /// Load filament to MMU
void M705(); /// Eject filament from MMU
void M706(); /// Cut filament by MMU
void M707(); /// Read variable from MMU
void M708(); /// Write variable to MMU
void M709(); /// MMU turn on/off/reset

void M930();
void M931();
void M932();
void M997(); /// M997 Update firmware. Prusa STM32 platform specific
void M999(); /// M999 reset MCU. Prusa STM32 platform specific

void M1587(); /// Wi-Fi credentials
void M1600(); /// Menu change filament. Prusa STM32 platform specific
void M1700(); /// Preheat. Prusa STM32 platform specific
void M1701(); /// Autoload. Prusa STM32 platform specific

}
