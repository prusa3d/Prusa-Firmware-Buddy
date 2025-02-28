/**
 * @file hw_configuration.hpp
 * @brief configuration of MK3.5
 */

/*
+-----++----------------------------------------------+-------------------------------+-------------------+
|pin  || MK4 xBuddy027 (with R191 0R)                 | MK4 xBuddy037                 | MK3.5 xBuddy037   |
+-----++----------------------------------------------+-------------------------------+-------------------+
|PA6  || Heatbreak temp                               | Heatbreak temp                | FILAMENT_SENSOR   |
|PA10 || accel CS                                     | accel CS                      | TACHO0            |
|PE9  || FAN1 PWM inverted                            | FAN1 PWM                      | FAN1 PWM          |
|PE10 || both fans tacho                              | both fans tacho               | TACHO1            |
|PE11 || FAN0 PWM inverted                            | FAN0 PWM                      | FAN0 PWM          |
|PE14 || NOT CONNECTED == same as MK4 on 037 (pullup) | EXTRUDER_SWITCH .. use pullup | EXTRUDER_SWITCH   |
|PF13 || eeprom, fan multiplexor                      | eeprom, fan multiplexor       | ZMIN (PINDA)      |
+-----++----------------------------------------------+-------------------------------+-------------------+


PC0 HOTEND_NTC is the same for all versions, but needs EXTRUDER_SWITCH enabled to provide pullup for MK3.5

xBuddy037 FW related changes
disconnected power panic cable will cause power panic
current measurement changed from 5V to 3V3 - need to recalculate values
MMU switching changed - no need to generate pulses anymore
MMU_RESET logic inverted
*/

#pragma once
#include "hw_configuration_common.hpp"

namespace buddy::hw {
class Configuration : public ConfigurationCommon {
    Configuration();
    Configuration(const Configuration &) = delete;

    bool error__loveboard_detected = true; // mk3.5 does not have loveboard, detecting it means wrong HW

public:
    /**
     * @brief Meyers singleton
     * @return Configuration&
     */
    static Configuration &Instance();

    static constexpr bool has_inverted_fans() { return false; }
    static constexpr bool has_inverted_mmu_reset() { return true; }
    static constexpr bool has_mmu_power_up_hw() { return true; }
    static constexpr bool needs_push_pull_mmu_reset_pin() { return true; }
    static constexpr bool has_trinamic_oscillators() { return true; }
    static constexpr bool needs_software_mmu_powerup() { return true; }

    /**
     * @brief voltage reference of current measurement
     * Allegro ACS711KEXLT-15AB
     * +-15 A, 90mV/A, 0A -> output == Vcc/2
     *
     * XBuddy rev37 3V3 reference
     * @return float current [mA]
     */
    static constexpr float curr_measurement_voltage_to_current(float voltage) {
        constexpr float allegro_curr_from_voltage = 1 / 0.09F;
        const float allegro_zero_curr_voltage = 3.35F / 2.F; // choose half of 3V3 range
        return (voltage - allegro_zero_curr_voltage) * allegro_curr_from_voltage;
    }

    bool is_fw_incompatible_with_hw();
};

} // namespace buddy::hw
