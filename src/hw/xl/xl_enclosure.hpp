/**
 * @file xl_enclosure.hpp
 * @brief Class handling XL enclosure
 */

#pragma once

#include <optional>
#include <array>
#include <general_response.hpp>

#include <temperature.hpp>
#include "marlin_server_shared.h"
#include "client_fsm_types.h"
#include <general_response.hpp>

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
 * MCU cooling has a priority and is activated on temperatures over 85*C and deactivated after cooldown under 70*C
 * If print filtration is enabled, enclosure fan is activated the whole print.
 * After print ends, fan is ventilation for another 1-10 minutes based on printed material.
 *
 * The HEPA filter has 600h lifespan. After that reminder for 5 days can be activated. Warning is issued 100h before end of its life with the link to eshop.
 */

class Enclosure {
public:
    static constexpr int64_t expiration_deadline_sec = 600 * 3600;
    static constexpr int64_t expiration_warning_sec = 500 * 3600;
    static constexpr const int MIN_FAN_PWM = 50;

    Enclosure();
    std::optional<buddy::Temperature> getEnclosureTemperature();

    void setEnabled(bool enable); // enable enclosure
    void setPrintFiltration(bool enable); /** Fan is spinning with manual rpm (80% power on default) for the whole print */
    void setPostPrintFiltration(bool enable); /** Base on the used materials - fan will filter out the enclosure after the print */

    /**
     *  Set RPM percentage defined by user in Manual Settings. This value applies on AlwaysOn & PostPrint modes
     */
    void setUserFanRPM(uint8_t value) {
        user_rpm = value;
        config_store().xl_enclosure_fan_manual.set(value);
    }

    /**
     *  Set post print filtration duration in Manual Settings.
     */
    void setPostPrintFiltrationDuration(uint8_t value) {
        post_print_duration_sec = value * 60 /* stored in minutes */;
        config_store().xl_enclosure_post_print_duration.set(value);
    }

    /** Enclosure loop function embedded in marlin_server
     * Handling timers and enclosure fan.
     *
     * @param MCU_modular_bed_temp [in] - MCU Temperature for handling fan cooling/filtration
     * @param active_dwarf_board_temp [in] - Current or first dwarf board temperature
     * @param print_state [in]
     * @return WarningType for GUI
     */
    std::optional<WarningType> loop(int32_t MCU_modular_bed_temp, int16_t active_dwarf_board_temp, marlin_server::State print_state);

    /**
     *  Set up reminder for expired filter change
     * @param response [in] - User's response on filter expiration [Ignore], [Postpone 5 days], [Done]
     */
    void setUpReminder(Response response);

    /**
     *  Reset filter expiration timer and both flags for warning and expiration dialogs
     */
    void resetFilterTimer();

    // FLAG GETTER FUNCTION
    inline bool isEnabled() const { return persistent_flags & PERSISTENT::ENABLED; }
    inline bool isPrintFiltrationEnabled() const { return persistent_flags & PERSISTENT::PRINT_FILTRATION; }
    inline bool isPostPrintFiltrationEnabled() const { return persistent_flags & PERSISTENT::POST_PRINT_FILTRATION; }
    inline bool isWarningShown() const { return persistent_flags & PERSISTENT::WARNING_SHOWN; }
    inline bool isExpirationShown() const { return persistent_flags & PERSISTENT::EXPIRATION_SHOWN; }
    inline bool isReminderSet() const { return persistent_flags & PERSISTENT::REMINDER_5DAYS; }

    inline bool isActive() const { return active_mode == EnclosureMode::Active; }

    inline bool isPrinting() const { return runtime_flags & RUNTIME::PRINTING; }
    inline bool isCooling() const { return runtime_flags & RUNTIME::ACTIVE_COOLING; }
    inline bool isPostPrintFiltrationActive() const { return runtime_flags & RUNTIME::ACTIVE_POST_PRINT_FILTRATION; }
    inline bool isTemperatureValid() const { return runtime_flags & RUNTIME::TEMP_VALID; }

    struct PERSISTENT {
        static constexpr uint8_t ENABLED = 0x01;
        static constexpr uint8_t PRINT_FILTRATION = 0x02;
        static constexpr uint8_t WARNING_SHOWN = 0x04;
        static constexpr uint8_t EXPIRATION_SHOWN = 0x08;
        static constexpr uint8_t POST_PRINT_FILTRATION = 0x10;
        static constexpr uint8_t REMINDER_5DAYS = 0x20;
    };

    struct RUNTIME {
        static constexpr uint8_t PRINTING = 0x01;
        static constexpr uint8_t ACTIVE_COOLING = 0x02;
        static constexpr uint8_t ACTIVE_POST_PRINT_FILTRATION = 0x04;
        static constexpr uint8_t TEMP_VALID = 0x08;
    };

private:
    enum class FanMode {
        Off = 0,
        Test,
        User,
        Max,
    };

    enum class EnclosureMode {
        Idle = 0,
        Test,
        Active,
    };

    /**
     *  Set persistent flag and save it to EEPROM
     */
    void setPersistentFlg(uint8_t flg) {
        persistent_flags |= flg;
        config_store().xl_enclosure_flags.set(persistent_flags);
    }

    /**
     *  Clear persistent flag and save it to EEPROM
     */
    void clrPersistentFlg(uint8_t flg) {
        persistent_flags &= ~flg;
        config_store().xl_enclosure_flags.set(persistent_flags);
    }

    void setRuntimeFlg(uint8_t flg) { runtime_flags |= flg; }
    void clrRuntimeFlg(uint8_t flg) { runtime_flags &= ~flg; }

    /**
     *  Get Fan PWM from active_mode and enclosure state
     */
    uint8_t getPwmFromMode() const;

    /**
     *  Test enclosure fan.
     *  @return True if test failed and fan is not behaving correctly.
     */
    bool testFanTacho();

    /**
     *  Test enclosure fan presence
     *  @return WarningType - returns WarningType::EnclosureFanError if test did not pass. Otherwise returns std::optional without value
     */
    std::optional<WarningType> testFanPresence(uint32_t curr_sec);

    /**
     *  Looks which filament are required for this print and set up post-print filtration period for the smelly ones
     *  @return True if there is at least one filament used, which is aligable for post print filtration
     */
    bool isPostPrintFiltrationNeeded();

    /**
     *  Update and check filter expiration timer. On 500. & 600. hour of printing notification to GUI is sent
     */
    std::optional<WarningType> updateFilterExpirationTimer(uint32_t delta_sec);

    /**
     *  Timing validation period of recorded temperature: 5 minutes
     */
    void updateTempValidationTimer(uint32_t curr_sec);

    /**
     *  Post print filtration based on material list
     *  @return True if time is up
     */
    bool updatePostPrintFiltrationTimer(uint32_t curr_sec);

    /**
     *  Handles fan activation during printing state
     *  @return optional with no value or WarningType::EnclosureFilterExpiration
     */
    std::optional<WarningType> checkPrintState(marlin_server::State print_state, uint32_t curr_sec);

    std::atomic<uint8_t> persistent_flags;
    std::atomic<uint8_t> runtime_flags;
    std::atomic<uint8_t> user_rpm;
    std::atomic<uint32_t> post_print_duration_sec;

    EnclosureMode active_mode;
    uint32_t last_sec;
    uint32_t last_timer_update_sec;
    uint32_t print_start_sec;
    uint32_t print_end_sec;
    uint32_t filter_postpone_sec;
    uint32_t fan_presence_test_sec;
    marlin_server::State previous_print_state;

    std::atomic<std::optional<buddy::Temperature>> active_dwarf_board_temp;
};

extern Enclosure xl_enclosure;
