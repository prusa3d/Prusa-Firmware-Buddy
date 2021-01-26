#pragma once

namespace PrusaGcodeSuite {
void G26();  /// first layer calibration
void G162(); /// handler-specific configuration

void M999(); /// M999 reset MCU. Prusa STM32 platform specific

void M1400(); /// Preheat. Prusa STM32 platform specific
}
