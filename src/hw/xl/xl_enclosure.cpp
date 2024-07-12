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

static constexpr uint32_t footer_temp_delay_sec = 5 * 60;
static constexpr int64_t expiration_5day_reminder_period_sec = 5 * 24 * 3600;

static constexpr uint32_t tick_delay_sec = 1;
static constexpr uint32_t timers_update_period_sec = 60;
static constexpr uint32_t fan_presence_test_period_sec = 3;

static constexpr int32_t fan_start_temp_treshold = 80; // °C
static constexpr int32_t fan_stop_temp_treshold = 75; // °C
static constexpr int32_t dwarf_board_temp_model_difference = -15; // °C

static constexpr const int pwm_on_50_percent = (FANCTLENCLOSURE_PWM_MAX * 50) / 100;

Enclosure xl_enclosure;

Enclosure::Enclosure()
    : persistent_flags(config_store().xl_enclosure_flags.get())
    , runtime_flags(0)
    , user_rpm(config_store().xl_enclosure_fan_manual.get())
    , post_print_duration_sec(config_store().xl_enclosure_post_print_duration.get() * 60 /* stored in minutes */)
    , active_mode(EnclosureMode::Idle)
    , print_start_sec(0)
    , print_end_sec(0)
    , filter_postpone_sec(0)
    , fan_presence_test_sec(0)
    , previous_print_state(marlin_server::State::Idle)
    , active_dwarf_board_temp(INVALID_TEMPERATURE) {

    // Set up timers
    last_timer_update_sec = last_sec = ticks_s();
}

std::optional<WarningType> Enclosure::testFanPresence(uint32_t curr_tick) {
    std::optional<WarningType> ret = std::nullopt;
    if (curr_tick - fan_presence_test_sec >= fan_presence_test_period_sec) {
        if (Fans::enclosure().getRPMIsOk()) {
            setPersistentFlg(PERSISTENT::ENABLED);
            active_mode = EnclosureMode::Active;
        } else {
            clrPersistentFlg(PERSISTENT::ENABLED);
            active_mode = EnclosureMode::Idle;
            ret = { WarningType::EnclosureFanError };
        }
        fan_presence_test_sec = 0;
    }
    return ret;
}

bool Enclosure::testFanTacho() {
    if (isEnabled() && Fans::enclosure().getPWM() != 0) {
        auto state = Fans::enclosure().getState();
        return (state == CFanCtlCommon::FanState::running || state == CFanCtlCommon::FanState::error_running || state == CFanCtlCommon::FanState::error_starting) && !Fans::enclosure().getRPMIsOk();
    }
    return false; // Valid behaviour can be checked only with active fan
}

void Enclosure::setEnabled(bool enable) {
    if (enable) {
        setPersistentFlg(PERSISTENT::ENABLED);
    } else {
        clrPersistentFlg(PERSISTENT::ENABLED);
    }
}

void Enclosure::setPrintFiltration(bool enable) {
    if (enable) {
        setPersistentFlg(PERSISTENT::PRINT_FILTRATION);
    } else {
        clrPersistentFlg(PERSISTENT::PRINT_FILTRATION);
    }
}

void Enclosure::setPostPrintFiltration(bool enable) {
    if (enable) {
        setPersistentFlg(PERSISTENT::POST_PRINT_FILTRATION);
    } else {
        clrPersistentFlg(PERSISTENT::POST_PRINT_FILTRATION);
    }
}

int Enclosure::getEnclosureTemperature() {
    if (isTemperatureValid() && active_dwarf_board_temp != INVALID_TEMPERATURE) {
        return active_dwarf_board_temp + dwarf_board_temp_model_difference;
    }
    return INVALID_TEMPERATURE;
}

void Enclosure::resetFilterTimer() {
    clrPersistentFlg(PERSISTENT::WARNING_SHOWN | PERSISTENT::EXPIRATION_SHOWN | PERSISTENT::REMINDER_5DAYS);
    config_store().xl_enclosure_filter_timer.set(0);
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
            setPersistentFlg(PERSISTENT::REMINDER_5DAYS);
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
        setRuntimeFlg(RUNTIME::TEMP_VALID);
    }
}

bool Enclosure::updatePostPrintFiltrationTimer(uint32_t curr_sec) {
    bool times_up = curr_sec - print_end_sec >= post_print_duration_sec;
    if (times_up) {
        print_end_sec = 0;
    }
    return times_up;
}

// Expiration timer + Expiration warning timer
// expiration_shown flag and xl_enclosure_filter_timer EEPROM value are reused for 5 day reminder
std::optional<WarningType> Enclosure::updateFilterExpirationTimer(uint32_t delta_sec) {
    std::optional<WarningType> ret = std::nullopt;
    int64_t expiration_timer = config_store().xl_enclosure_filter_timer.get();

    if (isExpirationShown()) {
        // 5 day reminder after filter already expired (RTC time)
        if (isReminderSet() && time(nullptr) - expiration_timer >= expiration_5day_reminder_period_sec) {
            ret = { WarningType::EnclosureFilterExpiration };
        }
        return ret;
    }

    // check filter expiration
    if (!isWarningShown() && expiration_timer + delta_sec >= expiration_warning_sec) {
        setPersistentFlg(PERSISTENT::WARNING_SHOWN);
        ret = { WarningType::EnclosureFilterExpirWarning };
    } else if (!isExpirationShown() && expiration_timer + delta_sec >= expiration_deadline_sec) {
        setPersistentFlg(PERSISTENT::EXPIRATION_SHOWN);
        return { WarningType::EnclosureFilterExpiration };
    }

    expiration_timer += delta_sec;
    config_store().xl_enclosure_filter_timer.set(expiration_timer);
    return ret;
}

bool Enclosure::isPostPrintFiltrationNeeded() {
    if (GCodeInfo::getInstance().UsedExtrudersCount() <= 0) {
        return false;
    }

    EXTRUDER_LOOP() { // e == physical_extruder
        auto &extruder_info = GCodeInfo::getInstance().get_extruder_info(e);
        if (!extruder_info.used() || !extruder_info.filament_name.has_value()) {
            continue;
        }

        if (FilamentType::from_name(extruder_info.filament_name.value().data()).parameters().requires_filtration) {
            return true;
        }
    }
    return false;
}

// The previous state is not more narrowly defined here, because enclosure loop is executed periodically with a delay
// More state switches could take place between loop executions
static bool isBasicStartPrint(marlin_server::State previous_state) {
    switch (previous_state) {
    case marlin_server::State::Idle:
    case marlin_server::State::WaitGui:
    case marlin_server::State::PrintPreviewInit:
    case marlin_server::State::PrintPreviewImage:
    case marlin_server::State::PrintPreviewConfirmed:
    case marlin_server::State::PrintPreviewQuestions:
    case marlin_server::State::PrintPreviewToolsMapping:
    case marlin_server::State::PrintInit:
    case marlin_server::State::SerialPrintInit:
        return true;
    default:
        break;
    }
    return false;
}

static bool isPausing(marlin_server::State state) {
    switch (state) {
    case marlin_server::State::Pausing_Begin:
    case marlin_server::State::Pausing_Failed_Code:
    case marlin_server::State::Pausing_WaitIdle:
    case marlin_server::State::Pausing_ParkHead:
    case marlin_server::State::Paused:
        return true;
    default:
        break;
    }
    return false;
}

static bool isResuming(marlin_server::State state) {
    // Just to be sure, more state switches could take place between loop executions
    if (isPausing(state)) {
        return true;
    }

    switch (state) {
    case marlin_server::State::Resuming_Begin:
    case marlin_server::State::Resuming_Reheating:
    case marlin_server::State::Resuming_UnparkHead_XY:
    case marlin_server::State::Resuming_UnparkHead_ZE:
        return true;
    default:
        break;
    }
    return false;
}

std::optional<WarningType> Enclosure::checkPrintState(marlin_server::State print_state, uint32_t curr_sec) {

    std::optional<WarningType> ret = std::nullopt;
    if (print_state == marlin_server::State::Printing) {
        if (!isPrinting()) {
            // PRINT STARTED - can be Start / Resume / Recovery
            setRuntimeFlg(RUNTIME::PRINTING);
            // Do not invalidate on Resume
            if (!isResuming(previous_print_state)) {
                clrRuntimeFlg(RUNTIME::TEMP_VALID);
            }
            print_start_sec = curr_sec;
            print_end_sec = 0;

            // Pop up expiration dialog
            if (isExpirationShown() && !isReminderSet() && isBasicStartPrint(previous_print_state)) {
                // Print start (except for resuming)
                ret = { WarningType::EnclosureFilterExpiration };
            }
        }
    } else {
        if (isPrinting()) {
            // PRINT ENDED - can be Pause / Abort / Crash / Power Panic
            // Do not invalidate temperature on Pause
            if (!isPausing(print_state)) {
                clrRuntimeFlg(RUNTIME::TEMP_VALID);
                print_start_sec = 0;
            }
            clrRuntimeFlg(RUNTIME::PRINTING);
            if (isPostPrintFiltrationEnabled() && isPostPrintFiltrationNeeded()) {
                setRuntimeFlg(RUNTIME::ACTIVE_POST_PRINT_FILTRATION);
                print_end_sec = curr_sec;
            }
        } else if (print_state == marlin_server::State::Aborted && isTemperatureValid()) {
            clrRuntimeFlg(RUNTIME::TEMP_VALID);
        }
    }
    return ret;
}

uint8_t Enclosure::getPwmFromMode() const {

    switch (active_mode) {

    case EnclosureMode::Idle:
        return isEnabled() ? pwm_on_50_percent : 0;

    case EnclosureMode::Test:
        return pwm_on_50_percent;

    case EnclosureMode::Active:
        if (isCooling()) {
            return FANCTLENCLOSURE_PWM_MAX;
        }

        if ((isPrintFiltrationEnabled() && isPrinting()) || (isPostPrintFiltrationEnabled() && isPostPrintFiltrationActive())) {
            return (FANCTLENCLOSURE_PWM_MAX * user_rpm) / 100;
        }

        return 0;
    }

    return 0;
}

std::optional<WarningType> Enclosure::loop(int32_t MCU_modular_bed_temp, int16_t dwarf_board_temp, marlin_server::State print_state) {

    // 1s loop delay
    uint32_t curr_sec = ticks_s();
    if (curr_sec - last_sec < tick_delay_sec) {
        return std::nullopt;
    }

    last_sec = curr_sec;
    active_dwarf_board_temp = dwarf_board_temp; // update actual temp of active dwarf board

    std::optional<WarningType> warning_opt = checkPrintState(print_state, curr_sec);
    previous_print_state = print_state;

    // Deactivated enclosure
    if (!isEnabled() && active_mode != EnclosureMode::Idle) {
        active_mode = EnclosureMode::Idle;
        clrRuntimeFlg(RUNTIME::ACTIVE_COOLING | RUNTIME::ACTIVE_POST_PRINT_FILTRATION | RUNTIME::TEMP_VALID);
    }

    switch (active_mode) {
    case EnclosureMode::Idle:

        if (isEnabled()) {
            active_mode = EnclosureMode::Test;
            fan_presence_test_sec = curr_sec;
        }
        break;

    case EnclosureMode::Test:

        warning_opt = testFanPresence(curr_sec);
        break;

    case EnclosureMode::Active: {

        if (testFanTacho()) {
            clrPersistentFlg(PERSISTENT::ENABLED);
            clrRuntimeFlg(RUNTIME::ACTIVE_COOLING | RUNTIME::ACTIVE_POST_PRINT_FILTRATION | RUNTIME::TEMP_VALID);
            warning_opt = { WarningType::EnclosureFanError };
            break;
        }

        // Update temperature validation timer (even during MCU Cooling)
        if (!isTemperatureValid() && isPrinting() && print_start_sec != 0) {
            updateTempValidationTimer(curr_sec);
        }

        // MCU Cooling has a priority and cannot be overriden by Print Filtration / Post Print Filtration / Users manual RPM change
        if (!isCooling() && MCU_modular_bed_temp >= fan_start_temp_treshold) {
            setRuntimeFlg(RUNTIME::ACTIVE_COOLING);
        } else if (isCooling() && MCU_modular_bed_temp < fan_stop_temp_treshold) {
            clrRuntimeFlg(RUNTIME::ACTIVE_COOLING);
        }

        if (isCooling() || (isPrintFiltrationEnabled() && isPrinting())) {
            break;
        }

        if (isPostPrintFiltrationEnabled() && isPostPrintFiltrationActive()) {
            if (print_end_sec == 0 || updatePostPrintFiltrationTimer(curr_sec)) {
                clrRuntimeFlg(RUNTIME::ACTIVE_POST_PRINT_FILTRATION | RUNTIME::TEMP_VALID);
            }
        }

    } break;
    }

    // Control Fan PWM
    const uint8_t fan_pwm = getPwmFromMode();
    Fans::enclosure().setPWM(fan_pwm);

    if (!isActive() || fan_pwm == 0) {
        last_timer_update_sec = curr_sec;
        return warning_opt;
    }

    // Check timer every minute of active fan - longer period because it writes to EEPROM
    if (curr_sec - last_timer_update_sec >= timers_update_period_sec) {
        // Filter expiration notification
        std::optional<WarningType> expir_warning = updateFilterExpirationTimer(curr_sec - last_timer_update_sec);
        if (expir_warning.has_value()) {
            warning_opt = expir_warning;
        }
        last_timer_update_sec = curr_sec;
    }
    return warning_opt;
}
