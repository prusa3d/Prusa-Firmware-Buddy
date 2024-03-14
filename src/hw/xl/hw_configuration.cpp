/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "otp.hpp"
#include "adc.hpp"
#include <limits>

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration();
    return ths;
}

SandwichConfiguration &SandwichConfiguration::Instance() {
    static SandwichConfiguration ths = SandwichConfiguration();
    return ths;
}

SandwichConfiguration::SandwichConfiguration() {
    revision = read_revision();
}

uint8_t SandwichConfiguration::read_revision() const {
    // This needs all ADC channels to be read first
    PowerHWIDAndTempMux.read_all_channels();

    // Get raw revision counter from analog bits
    // Currently we support only binary values
    constexpr uint16_t high_threshold = AdcDma1::sample_max / 2;
    const bool bit0 = AdcGet::hwId0() < high_threshold;
    const bool bit1 = AdcGet::hwId1() < high_threshold;
    const bool bit2 = AdcGet::hwId2() < high_threshold;
    const uint8_t raw_revision = (bit0 << 0) | (bit1 << 1) | (bit2 << 2);

    return [&raw_revision]() -> uint8_t {
        switch (raw_revision) {
        case 0:
            return 6;
        case 1:
            return 8;
        case 2:
            return 9;
        case 3:
            return 10;
        default:
            // If unknown expect the newest supported
            return 10;
        }
    }();
}

float SandwichConfiguration::divider_5V_coefficient() const {
    if (revision < 7) {
        constexpr float R1 = 10.0f * 1000.0f;
        constexpr float R2 = 1.0f * 1000.0f;
        return (R1 + R2) / R2;
    } else {
        constexpr float R1 = 10.0f * 1000.0f;
        constexpr float R2 = 15.0f * 1000.0f;
        return (R1 + R2) / R2;
    }
}

float SandwichConfiguration::divider_current_coefficient() const {
    if (revision < 7) {
        return 1.0f;
    } else {
        constexpr float R1 = 5.11f * 1000.0f;
        constexpr float R2 = 15.0f * 1000.0f;
        return (R1 + R2) / R2;
    }
}

} // namespace buddy::hw
