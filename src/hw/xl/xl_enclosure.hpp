/**
 * @file xl_enclosure.hpp
 * @brief Class handling XL enclosure
 */

#pragma once

#include <optional>
#include <array>
#include <atomic>
#include <cmath>
#include <general_response.hpp>
#include <pwm_utils.hpp>
#include <temperature.hpp>
#include "marlin_server_shared.h"
#include "client_fsm_types.hpp"
#include <general_response.hpp>
#include <warning_type.hpp>

/*
 *  Timers    - description                                - measuring time during
 *  ==============================================================================
 *  5 days    - filter change dialog postponed             - real time (even if printer is off)
 *  600 hours - filter change                              - fan active
 *  500 hours - filter change warning                      - fan active
 *  5 minutes - show enclosure temperature in footer       - printing
 *  X minutes - after print (X based on material)          - after printing is done
 */

/** @class Enclosure for XL
 * Handling timers for GUI, timers for filtration and filter expiration.
 * There are 2 modes:  MCU cooling (enabled) & enclosure chamber filtration (print filtration)
 * MCU cooling has a priority and is activated on temperatures over 80*C and deactivated after cooldown under 75*C
 * Print filtration is controlled by chamber filtration and set up by the user (default 40% on smelly filaments).
 * After print ends, fan is ventilation for another 1-30 minutes based on printed material / user preference.
 *
 * The HEPA filter has 600h lifespan. After that reminder for 5 days can be activated. Warning is issued 100h before end of its life with the link to eshop.
 */

class Enclosure {
public:
    Enclosure();
    std::optional<buddy::Temperature> get_enclosure_temperature();

    /**
     *  Set persistent flag and save it to EEPROM
     */
    void set_enabled(bool set);

    /** Enclosure loop function embedded in marlin_server
     * Handling timers and enclosure fan.
     *
     * @param mcu_modular_bed_temp [in] - MCU Temperature for handling fan cooling/filtration
     * @param active_dwarf_board_temp [in] - Current or first dwarf board temperature
     * @param active_nozzle_temp [in] - Nozzle temperature of the same dwarf
     */
    void loop(int32_t mcu_modular_bed_temp, int16_t active_dwarf_board_temp, float active_nozzle_temp);

    inline bool is_enabled() const { return is_enabled_; }
    inline bool is_active() const { return active_mode == EnclosureMode::Active; }

private:
    enum class EnclosureMode {
        Idle = 0,
        Test,
        Active,
    };

    /**
     *  Get Fan PWM from active_mode and enclosure state
     *  @param mcu_modular_bed_temp can override pwm for cooling purposes if overheated
     */
    PWM255 calculate_pwm(int32_t mcu_modular_bed_temp);

    /**
     *  Test enclosure fan presence
     */
    void test_fan_presence(uint32_t curr_sec);

    /**
     *  Timing validation period of recorded temperature: 5 minutes
     */
    void update_temp_validation_timer();

    /**
     *  Estimate the chamber temperature from the dwarf board temperature
     *  and feed it to the smoothing filter
     */
    void update_enclosure_temperature(int16_t dwarf_board_temp, float nozzle_temp);

    /**
     *  Checks if modular bed is overheated and overwrites active_mode if it is
     *  @param mcu_modular_bed_temp
     */
    bool is_mcu_overheating(int32_t mcu_modular_bed_temp);

    static constexpr uint32_t tick_delay_sec = 1;

    EnclosureMode active_mode;
    uint32_t last_sec;
    uint32_t fan_presence_test_sec;

    bool is_enabled_ : 1 = false;
    bool is_mcu_overheated_ : 1 = false;
    bool is_temp_valid_ : 1 = false;

    std::atomic<buddy::Temperature> enclosure_temp_ = NAN;
};

extern Enclosure xl_enclosure;
