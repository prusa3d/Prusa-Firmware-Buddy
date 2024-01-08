/**
 * @file hw_configuration.hpp
 * @brief configuration of XL
 */

#pragma once
#include "hw_configuration_common.hpp"

namespace buddy::hw {
class Configuration : public ConfigurationCommon {
    Configuration() = default;
    Configuration(const Configuration &) = delete;

public:
    /**
     * @brief Meyers singleton
     * @return Configuration&
     */
    static Configuration &Instance();

    static bool is_fw_incompatible_with_hw() { return false; } // not incompatible does not mean compatible!
};

class SandwichConfiguration {
public:
    SandwichConfiguration();

    /**
     * This is singleton to copy the above Buddy configuration scheme.
     * Not a best option, but works if handled with care. Watch for:
     * - Unexpected initialization before the ADC/hw is ready to read the revision
     * - Race condition between implicit initialization and rest of the startup code.
     * - Unit tests that may end up being depended wia values passed in the singleton object
     */
    static SandwichConfiguration &Instance();

    /**
     * @brief Get the hardware revision of the Sandwich board
     *
     * See hardware documentation of revision differences
     *
     * @return uint8_t revision number
     */
    uint8_t get_revision() const {
        return revision;
    }

    /**
     * @brief Get coefficient 5V measurement voltage divider
     *
     * The 5V voltage is measured via a voltage divider. The voltage measured by
     * the ADC needs to be multiplied by this value to obtain real voltage on
     * the 5V power line.
     */
    float divider_5V_coefficient() const;

    /**
     * @brief Get coefficient 24V measurement voltage divider
     *
     * The voltage on the current sensing resistor is measured via a voltage
     * divider. The voltage on the resistor, as measured by the ADC, needs to be
     * multiplied by this value to obtain real voltage on the current sensing resistor.
     */
    float divider_current_coefficient() const;

private:
    uint8_t revision { 0 };
    uint8_t read_revision() const;
};

} // namespace buddy::hw
