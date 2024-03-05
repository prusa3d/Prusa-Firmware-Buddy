/**
 * @file xl_enclosure.cpp
 */

#include "xl_enclosure.hpp"
#include "timing.h"
#include "config_store/store_instance.hpp"
#include "fanctl.hpp"
#include "gcode_info.hpp"
#include "filament.hpp"
#include <ctime>
#include "option/development_items.h"

#define FLG_SET(VAR, FLAG)   VAR = VAR | (FLAG)
#define FLG_CLEAR(VAR, FLAG) VAR = VAR & ~(FLAG)

#if DEVELOPMENT_ITEMS()
static constexpr int64_t expiration_deadline_sec = 2 * 3600; // TEST values
static constexpr int64_t expiration_warning_sec = 1 * 3600; // TEST values
#else
static constexpr int64_t expiration_deadline_sec = 600 * 3600;
static constexpr int64_t expiration_warning_sec = 500 * 3600;
#endif
static constexpr uint32_t footer_temp_delay_sec = 5 * 60;
static constexpr uint32_t post_print_filtration_period_sec = 10 * 60;
static constexpr int64_t expiration_5day_reminder_period_sec = 5 * 24 * 3600;

static constexpr uint32_t tick_delay_sec = 1;
static constexpr uint32_t timers_update_period_sec = 60;
static constexpr uint32_t fan_presence_test_period_sec = 3;

static constexpr int32_t fan_start_temp_treshold = 80; // °C
static constexpr int32_t fan_stop_temp_treshold = 75; // °C
static constexpr int32_t dwarf_board_temp_model_difference = -15; // °C

static constexpr const int pwm_on_50_percent = (FANCTLENCLOSURE_PWM_MAX * 50) / 100;

static constexpr filament::Type filtration_filament_set[] = {
    filament::Type::ABS,
    filament::Type::ASA,
    filament::Type::PC,
    filament::Type::FLEX,
    filament::Type::HIPS,
    filament::Type::PP
};

static bool is_same(const char *curr_filament, const GCodeInfo::filament_buff &filament_type) {
    return strncmp(curr_filament, filament_type.begin(), filament_type.size()) == 0;
}

Enclosure xl_enclosure;

Enclosure::Enclosure()
    : config_flags(config_store().xl_enclosure_config_flags.get())
    , runtime_flags(0)
    , mode(EnclosureMode::IDLE)
    , post_print_timer_sec(0)
    , previous_print_state(marlin_server::State::Idle)
    , active_dwarf_board_temp(INVALID_TEMPERATURE) {

    // Set up timers
    last_timer_update_sec = last_sec = ticks_s();
    print_start_sec = print_end_sec = fan_presence_test_sec = 0;
}

std::optional<WarningType> Enclosure::testFanPresence(uint32_t curr_tick) {
    std::optional<WarningType> ret;
    if (fan_presence_test_sec == 0) {
        fan_presence_test_sec = curr_tick;
        Fans::enclosure().setPWM(pwm_on_50_percent);
        return ret;
    }
    if (curr_tick - fan_presence_test_sec >= fan_presence_test_period_sec) {
        if (Fans::enclosure().getRPMIsOk()) {
            mode = EnclosureMode::ACTIVE;
            config_store().xl_enclosure_config_flags.set(config_flags);
        } else {
            FLG_CLEAR(config_flags, CONFIG::ENABLED);
            config_store().xl_enclosure_config_flags.set(config_flags);
            mode = EnclosureMode::IDLE;
            ret = std::make_optional<WarningType>(WarningType::EnclosureFanError);
        }
        Fans::enclosure().setPWM(0);
        fan_presence_test_sec = 0;
    }
    return ret;
}

bool Enclosure::testFanTacho() {
    if (isEnabled() && isFanOn()) {
        auto state = Fans::enclosure().getState();
        return (state == CFanCtlCommon::FanState::running || state == CFanCtlCommon::FanState::error_running || state == CFanCtlCommon::FanState::error_starting) && !Fans::enclosure().getRPMIsOk();
    }
    return false; // Valid behaviour can be checked only with active fan
}

void Enclosure::setEnabled(bool enable) {
    if (enable) {
        FLG_SET(config_flags, CONFIG::ENABLED);
    } else {
        FLG_CLEAR(config_flags, CONFIG::ENABLED);
    }
    config_store().xl_enclosure_config_flags.set(config_flags);
}

void Enclosure::setAlwaysOn(bool enable) {
    if (enable) {
        FLG_SET(config_flags, CONFIG::ALWAYS_ON);
    } else {
        FLG_CLEAR(config_flags, CONFIG::ALWAYS_ON);
    }
    config_store().xl_enclosure_config_flags.set(config_flags);
}

void Enclosure::setPostPrint(bool enable) {
    if (enable) {
        FLG_SET(config_flags, CONFIG::POST_PRINT);
    } else {
        FLG_CLEAR(config_flags, CONFIG::POST_PRINT);
    }
    config_store().xl_enclosure_config_flags.set(config_flags);
}

int Enclosure::getEnclosureTemperature() {
    if (isTempValid() && active_dwarf_board_temp != INVALID_TEMPERATURE) {
        return active_dwarf_board_temp + dwarf_board_temp_model_difference;
    }
    return INVALID_TEMPERATURE;
}

void Enclosure::resetFilterTimer() {
    FLG_CLEAR(config_flags, CONFIG::WARNING_SHOWN | CONFIG::EXPIRATION_SHOWN);
    config_store().xl_enclosure_filter_timer.set(0);
    config_store().xl_enclosure_config_flags.set(config_flags);
}

void Enclosure::setUpReminder(Response response) {
    switch (response) {
    case Response::Done:
        resetFilterTimer();
        break;
    case Response::Postpone5Days:
        // RTC time is saved in EEPROM (xl_enclosure_filter_timer) on filter expiration
        // Check if RTC is initialized
        if (int64_t t = time(nullptr); t != static_cast<time_t>(-1)) {
            FLG_SET(config_flags, CONFIG::REMINDER_5DAYS);
            config_store().xl_enclosure_config_flags.set(config_flags);
            config_store().xl_enclosure_filter_timer.set(t); // Reuse filter counter for 5 day reminder - set timestamp of expiration
        } else {
            // Do not set REMINDER_5DAY flag, with uninitialized RTC there is no way to measure off-power time -> Expiration dialog pops up on every print start
            config_store().xl_enclosure_filter_timer.set(0);
        }

        break;
    case Response::Ignore:
    default:
        // Popup on each print start (except Resuming)
        // Expiration flag is already set & REMINDER_5DAYS should not be set
        break;
    }
}

void Enclosure::updateTempValidationTimer(uint32_t curr_sec) {
    if (curr_sec - print_start_sec >= footer_temp_delay_sec) {
        FLG_SET(runtime_flags, RUNTIME::TEMP_VALID);
        print_start_sec = 0;
    }
}

bool Enclosure::updatePostPrintFiltrationTimer(uint32_t curr_sec) {
    bool times_up = curr_sec - print_end_sec >= post_print_timer_sec;
    if (times_up) {
        print_end_sec = 0;
        post_print_timer_sec = 0;
    }
    return times_up;
}

// Expiration timer + Expiration warning timer
// expiration_shown flag and xl_enclosure_filter_timer EEPROM value are reused for 5 day reminder
std::optional<WarningType> Enclosure::updateFilterExpirationTimer(uint32_t delta_sec) {
    std::optional<WarningType> ret;
    int64_t expiration_timer = config_store().xl_enclosure_filter_timer.get();

    if (isExpirationShown()) {
        // 5 day reminder after filter already expired (RTC time)
        if (is5DayReminderSet() && time(nullptr) - expiration_timer >= expiration_5day_reminder_period_sec) {
            ret = std::make_optional<WarningType>(WarningType::EnclosureFilterExpiration);
        }
        return ret;
    }

    // check filter expiration
    if (!isWarningShown() && expiration_timer + delta_sec >= expiration_warning_sec) {
        FLG_SET(config_flags, CONFIG::WARNING_SHOWN);
        config_store().xl_enclosure_config_flags.set(config_flags);
        ret = std::make_optional<WarningType>(WarningType::EnclosureFilterExpirWarning);
    } else if (!isExpirationShown() && expiration_timer + delta_sec >= expiration_deadline_sec) {
        FLG_SET(config_flags, CONFIG::EXPIRATION_SHOWN);
        config_store().xl_enclosure_config_flags.set(config_flags);
        return std::make_optional<WarningType>(WarningType::EnclosureFilterExpiration);
    }

    expiration_timer += delta_sec;
    config_store().xl_enclosure_filter_timer.set(expiration_timer);
    return ret;
}

uint32_t Enclosure::setUpPostPrintFiltrationPeriod() {
    uint32_t t = 0;
    if (GCodeInfo::getInstance().UsedExtrudersCount() <= 0) {
        return t;
    }

    EXTRUDER_LOOP() { // e == physical_extruder
        auto &extruder_info = GCodeInfo::getInstance().get_extruder_info(e);
        if (!extruder_info.used() || !extruder_info.filament_name.has_value()) {
            continue;
        }

        // If any of the filaments in filtration_filament_set is used in the print -> set up post print filtration timer for 10 minutes (in seconds)
        for (uint32_t fil = 0; fil < sizeof(filtration_filament_set) / sizeof(filament::Type); fil++) {
            if (is_same(filament::get_name(filtration_filament_set[fil]), extruder_info.filament_name.value())) {
                return post_print_filtration_period_sec;
            }
        }
    }
    return t;
}

std::optional<WarningType> Enclosure::checkPrintState(marlin_server::State print_state, uint32_t curr_sec) {
    std::optional<WarningType> ret;
    if (print_state == marlin_server::State::Printing) {
        if (!isPrinting()) {
            // PRINT STARTED - can be Start / Resume / Recovery
            FLG_SET(runtime_flags, RUNTIME::PRINTING);
            FLG_CLEAR(runtime_flags, RUNTIME::TEMP_VALID);
            print_start_sec = curr_sec;
            print_end_sec = 0;
            post_print_timer_sec = setUpPostPrintFiltrationPeriod();

            // Pop up expiration dialog
            if (isExpirationShown() && !is5DayReminderSet() && (previous_print_state == marlin_server::State::PrintInit || previous_print_state == marlin_server::State::SerialPrintInit)) {
                // Print start (except for resuming)
                ret = std::make_optional<WarningType>(WarningType::EnclosureFilterExpiration);
            }
        }
    } else {
        if (isPrinting()) {
            // PRINT ENDED - can be Pause / Abort / Crash / Power Panic
            FLG_CLEAR(runtime_flags, RUNTIME::PRINTING | RUNTIME::TEMP_VALID);
            FLG_SET(runtime_flags, RUNTIME::POST_PRINT);
            print_end_sec = curr_sec;
            print_start_sec = 0;
        }
    }
    return ret;
}

uint8_t Enclosure::getFanPwm() {
    uint8_t percentage = config_store().xl_enclosure_fan_manual.get();
    FLG_CLEAR(runtime_flags, RUNTIME::RPM_CHANGE);
    return (FANCTLENCLOSURE_PWM_MAX * percentage) / 100;
}

std::optional<WarningType> Enclosure::loop(int32_t MCU_modular_bed_temp, int16_t dwarf_board_temp, marlin_server::State print_state) {
    std::optional<WarningType> ret;

    // 1s loop delay
    uint32_t curr_sec = ticks_s();
    if (curr_sec - last_sec < tick_delay_sec) {
        return ret;
    }

    last_sec = curr_sec;
    active_dwarf_board_temp = dwarf_board_temp; // update actual temp of active dwarf board

    ret = checkPrintState(print_state, curr_sec);
    previous_print_state = print_state;
    if (ret.has_value()) {
        return ret; // Expiration reminder on the start of every print
    }

    // Deactivate enclosure
    if (!isEnabled() && mode != EnclosureMode::IDLE) {
        mode = EnclosureMode::IDLE;
        FLG_CLEAR(runtime_flags, RUNTIME::FAN_ON | RUNTIME::COOLING | RUNTIME::POST_PRINT | RUNTIME::RPM_CHANGE | RUNTIME::TEMP_VALID);
        Fans::enclosure().setPWM(0);
    }

    switch (mode) {
    case EnclosureMode::IDLE:
        if (isEnabled()) {
            mode = EnclosureMode::TEST;
            fan_presence_test_sec = 0;
            testFanPresence(curr_sec);
        }
        return ret;
    case EnclosureMode::TEST:
        ret = testFanPresence(curr_sec);
        return ret;
    case EnclosureMode::ACTIVE:
        if (testFanTacho()) {
            FLG_CLEAR(config_flags, CONFIG::ENABLED);
            FLG_CLEAR(runtime_flags, RUNTIME::FAN_ON | RUNTIME::COOLING | RUNTIME::POST_PRINT | RUNTIME::TEMP_VALID);
            Fans::enclosure().setPWM(0);
            return WarningType::EnclosureFanError;
        }
        // Performing MCU cooling has a priority
        // Fan start threshold >= 80°C
        // Fan stop threshold < 75°C
        if (!isCooling() && MCU_modular_bed_temp >= fan_start_temp_treshold) {

            FLG_SET(runtime_flags, RUNTIME::COOLING | RUNTIME::FAN_ON);

            Fans::enclosure().setPWM(FANCTLENCLOSURE_PWM_MAX); // MCU cooling runs the fan at 100% speed
        } else if (isCooling() && MCU_modular_bed_temp < fan_stop_temp_treshold) {

            FLG_CLEAR(runtime_flags, RUNTIME::COOLING);

            if (isAlwaysOn() && (isPrinting() || isPostPrintActive())) {
                // FAN_ON stays true, continue with filtration
                Fans::enclosure().setPWM(getFanPwm());
            } else {
                FLG_CLEAR(runtime_flags, RUNTIME::FAN_ON);
                Fans::enclosure().setPWM(0);
            }
        }

        // Filtration (active fan) for the whole printing
        if (isAlwaysOn() && isPrinting() && !isFanOn()) {
            FLG_SET(runtime_flags, RUNTIME::FAN_ON);
            Fans::enclosure().setPWM(getFanPwm());
        }

        // React on manual rpm change (flag is cleared by getFanPwm)
        if (isFanOn() && isRMPChanged()) {
            Fans::enclosure().setPWM(getFanPwm());
        }

        // Update active fan timers
        if (!isTempValid() && isPrinting() && print_start_sec != 0) {
            updateTempValidationTimer(curr_sec);
        }

        // Post print filtration handles switching off the fan
        // It can be turned on/off and time period is calculated beforehand on print start by checkPrintState()
        if (isPostPrintActive() && (!isPostPrintEnabled() || print_end_sec == 0 || updatePostPrintFiltrationTimer(curr_sec))) {
            FLG_CLEAR(runtime_flags, RUNTIME::POST_PRINT | RUNTIME::FAN_ON | RUNTIME::TEMP_VALID);
            Fans::enclosure().setPWM(0);
        }

        break;
    default:
        break;
    }

    if (!isFanOn()) {
        last_timer_update_sec = curr_sec;
        FLG_CLEAR(runtime_flags, RUNTIME::TEMP_VALID);
        return ret;
    }

    // Check timer every minute of active FAN_ON - longer period because it writes to EEPROM
    if (curr_sec - last_timer_update_sec >= timers_update_period_sec) {
        // Filter expiration notification
        ret = updateFilterExpirationTimer(curr_sec - last_timer_update_sec);
        last_timer_update_sec = curr_sec;
    }
    return ret;
}
