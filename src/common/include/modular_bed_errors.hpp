#pragma once

#include <cstdint>

/**
 * @brief Shared between buddy and modular bed
 */
namespace modular_bed_shared::errors {

enum class SystemError : uint16_t {
    PreheatError = 0x0001,
    Mintemp = 0x0002,
    Maxtemp = 0x0004,
    ThermalRunaway = 0x0008,
    HeatbedletError = 0x0010,
    OverCurrent = 0x0020,
    UnexpectedCurrent = 0x0040,
};

enum class HeatbedletError : uint16_t {
    HeaterDisconnected = 0x0001,
    HeaterShortCircuit = 0x0002,
    TemperatureBelowMinimum = 0x0004,
    TemperatureAboveMaximum = 0x0008,
    TemperatureDropDetected = 0x0010,
    TemperaturePeakDetected = 0x0020,
    PreheatError = 0x0040,
    TestHeatingError = 0x0080,
    /// Heater or thermistor is connected to connector which should be unused
    HeaterConnected = 0x0100,
};

} // namespace modular_bed_shared::errors
