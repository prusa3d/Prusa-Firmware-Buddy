/**
 * @file hw_configuration.hpp
 * @brief run time configuration of MK4
 */

/*
+-----++-------------------------------------------------------------------------------+-------------------------+-------------------+
|pin  || MK4 xBuddy027                                                                 | MK4 xBuddy037           | MK3.5 xBuddy037   |
+-----++-------------------------------------------------------------------------------+-------------------------+-------------------+
|PA6  || Heatbreak temp                                                                | Heatbreak temp          | FILAMENT_SENSOR   |
|PA10 || accel CS                                                                      | accel CS                | TACHO0            |
|PE9  || FAN1 PWM                                                                      | FAN1 PWM inverted       | FAN1 PWM inverted |
|PE11 || FAN0 PWM                                                                      | FAN0 PWM inverted       | FAN0 PWM inverted |
|PE10 || both fans tacho / fan 0 tacho (old loveboard)                                 | both fans tacho         | TACHO1            |
|PE14 || NOT CONNECTED (requires R191 0R)  / fan 1 tacho (old loveboard + requires D6) | EXTRUDER_SWITCH         | EXTRUDER_SWITCH   |
|PF5  || -                                                                             | -                       | PINDA_THERM       | .. IGNORE
|PF13 || eeprom, fan multiplexor                                                       | eeprom, fan multiplexor | ZMIN (PINDA)      |
+-----++-------------------------------------------------------------------------------+-------------------------+-------------------+

LOVEBOARD support
need at least v31 (with EXTRUDER_SWITCH == multiplexer on fan tacho pins), xBuddy must have R191 0R instead of D6 to support 3V3 reference for LOVEBOARD
following table contains only differences with loveboard >= v31 and without old PINDA support (only SuperPINDA suported for MK3 extruder)
+-----++----------------------------------------------+-------------------------------+-------------------+
|pin  || MK4 xBuddy027 (with R191 0R)                 | MK4 xBuddy037                 | MK3.5 xBuddy037   |
+-----++----------------------------------------------+-------------------------------+-------------------+
|PA6  || Heatbreak temp                               | Heatbreak temp                | FILAMENT_SENSOR   |
|PA10 || accel CS                                     | accel CS                      | TACHO0            |
|PE9  || FAN1 PWM inverted                            | FAN1 PWM                      | FAN1 PWM          |
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

    LoveBoardEeprom loveboard_eeprom;
    OtpStatus loveboard_status;

public:
    /**
     * @brief Meyers singleton
     * @return Configuration&
     */
    static Configuration &Instance();

    const LoveBoardEeprom &get_love_board() const { return loveboard_eeprom; }

    const OtpStatus &get_loveboard_status() const { return loveboard_status; }

    bool has_inverted_fans() const { return get_board_bom_id() < 37; }

    bool has_inverted_mmu_reset() const { return get_board_bom_id() >= 37; }

    // xBuddy scheme says: Revisions older than 34 must use open drain only.
    bool needs_push_pull_mmu_reset_pin() const { return get_board_bom_id() >= 34; }

    bool can_power_up_mmu_without_pulses() const { return get_board_bom_id() >= 37; }

    bool has_trinamic_oscillators() const { return get_board_bom_id() >= 37; }

    /**
     * @brief voltage reference of current measurement
     * Allegro ACS711KEXLT-15AB
     * +-15 A, 90mV/A, 0A -> output == Vcc/2
     *
     * XBuddy 0.3.4 3V3 reference
     * XBuddy < 0.3.4 5V reference
     * @return float current [mA]
     */
    float curr_measurement_voltage_to_current(float voltage) const;

    bool is_fw_incompatible_with_hw(); // not incompatible does not mean compatible!
};

} // namespace buddy::hw
