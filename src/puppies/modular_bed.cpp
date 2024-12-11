#include "puppies/modular_bed.hpp"
#include "bsod.h"
#include <common/sensor_data.hpp>
#include <logging/log.hpp>
#include "metric.h"
#include "puppy/modularbed/PuppyConfig.hpp"
#include "timing.h"
#include <puppies/PuppyBootstrap.hpp>
#include "otp.hpp"
#include "power_panic.hpp"
#include "printers.h"
#include <utility_extensions.hpp>
#include <i18n.h>
#include <option/is_knoblet.h>
#include <cassert>
#include <limits>
#include <numeric>
#include <stddef.h>
#include <stdint.h>
#include <tuple>
#include <freertos/mutex.hpp>

namespace buddy::puppies {

using Lock = std::unique_lock<freertos::Mutex>;

LOG_COMPONENT_DEF(ModularBed, logging::Severity::info);

METRIC_DEF(metric_state, "bed_state", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_currents, "bed_curr", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
METRIC_DEF(metric_states, "bedlet_state", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_temps, "bedlet_temp", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_targets, "bedlet_target", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_pwms, "bedlet_pwm", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_regulators, "bedlet_reg", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_bedlet_currents, "bedlet_curr", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_mcu_temperature, "bed_mcu_temp", METRIC_VALUE_FLOAT, 0, METRIC_HANDLER_DISABLE_ALL);

ModularBed::ModularBed(PuppyModbus &bus, uint8_t modbus_address)
    : ModbusDevice(bus, modbus_address) {}

CommunicationStatus ModularBed::ping() {
    Lock guard(mutex);
    return bus.read(unit, general_status);
}

CommunicationStatus ModularBed::initial_scan() {
    Lock guard(mutex);
    // Update static values
    CommunicationStatus status = bus.read(unit, general_static);
    if (status != CommunicationStatus::ERROR) {
        log_info(ModularBed, "HwBomId: %d", general_static.value.hw_bom_id);
        log_info(ModularBed, "HwOtpTimestsamp: %" PRIu32, general_static.value.hw_otp_timestamp);

        serial_nr_t sn = {}; // Last byte has to be '\0'
        static constexpr uint16_t raw_datamatrix_regsize = ftrstd::to_underlying(SystemInputRegister::hw_raw_datamatrix_last)
            - ftrstd::to_underlying(SystemInputRegister::hw_raw_datamatrix_first) + 1;
        // Check size of text -1 as the terminating \0 is not sent
        static_assert((raw_datamatrix_regsize * sizeof(uint16_t)) == sn.size() - 1, "Size of raw datamatrix doesn't fit modbus registers");

        for (uint16_t i = 0; i < raw_datamatrix_regsize; ++i) {
            sn[i * 2] = general_static.value.hw_data_matrix[i] & 0xff;
            sn[i * 2 + 1] = general_static.value.hw_data_matrix[i] >> 8;
        }
        log_info(ModularBed, "HwDatamatrix: %s", sn.begin());

        log_info(ModularBed, "Bedlet count: %d", general_static.value.heatbedlet_count);
        assert(general_static.value.heatbedlet_count == BEDLET_COUNT);
    } else {
        log_error(ModularBed, "Failed to read static general register pack");
    }

    // Reset dirty flag / schedule rewrite for important registers
    print_fan_active.dirty = true;
    bedlet_target_temp.dirty = true;

    return status;
}

CommunicationStatus ModularBed::refresh() {
    Lock guard(mutex);
    static uint32_t refresh_nr = 0;

    typedef CommunicationStatus (ModularBed::*MethodType)();
    static constexpr MethodType funcs[] = {
        &ModularBed::write_clear_fault_status,
        &ModularBed::write_reset_overcurrent,
        &ModularBed::write_test_heating,
        &ModularBed::write_print_fan_active,
        &ModularBed::read_general_status,
        &ModularBed::read_general_ready,
        &ModularBed::read_currents,
        &ModularBed::read_bedlet_data,
        &ModularBed::read_general_fault,
        &ModularBed::write_bedlet_target_temp,
        &ModularBed::read_bedlet_measured_max_current,
        &ModularBed::read_mcu_temperature,
    };
    if (++refresh_nr >= std::size(funcs)) {
        refresh_nr = 0;
    }
    return (this->*funcs[refresh_nr])();
}

CommunicationStatus ModularBed::write_clear_fault_status() {
    return bus.write(unit, clear_fault_status);
}

CommunicationStatus ModularBed::write_reset_overcurrent() {
    return bus.write(unit, reset_overcurrent);
}

CommunicationStatus ModularBed::write_test_heating() {
    return bus.write(unit, test_heating);
}

CommunicationStatus ModularBed::write_print_fan_active() {
    return bus.write(unit, print_fan_active);
}

CommunicationStatus ModularBed::read_general_status() {
    CommunicationStatus status = bus.read(unit, general_status, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    log_debug(ModularBed, "Panic: %d", general_status.value.power_panic_status);
    log_debug(ModularBed, "Current fault: %d", general_status.value.current_fault_status);

    // Report panic and fault problems only when not in true system-wide power panic
    if (!power_panic::ac_fault_triggered && !power_panic::is_ac_fault_active()) {
        if (general_status.value.current_fault_status) {
            fatal_error(ErrCode::ERR_ELECTRO_MB_FAULT);
        }
        if (general_status.value.power_panic_status) {
            fatal_error(ErrCode::ERR_ELECTRO_MB_PANIC);
        }
    }
    return status;
}

CommunicationStatus ModularBed::read_general_ready() {
    CommunicationStatus status = bus.read(unit, general_ready, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    log_debug(ModularBed, "Heatbedlets ready: %d", general_ready.value);
    return status;
}

CommunicationStatus ModularBed::read_currents() {
    CommunicationStatus status = bus.read(unit, currents, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    metric_record_custom(
        &metric_currents,
        ",n=0 v=%.3f,e=%.3f",
        static_cast<double>(currents.value.A_measured) / MODBUS_CURRENT_REGISTERS_SCALE,
        static_cast<double>(currents.value.A_expected) / MODBUS_CURRENT_REGISTERS_SCALE);
    metric_record_custom(
        &metric_currents,
        ",n=1 v=%.3f,e=%.3f",
        static_cast<double>(currents.value.B_measured) / MODBUS_CURRENT_REGISTERS_SCALE,
        static_cast<double>(currents.value.B_expected) / MODBUS_CURRENT_REGISTERS_SCALE);
    return status;
}

CommunicationStatus ModularBed::read_bedlet_data() {
    CommunicationStatus status = bus.read(unit, bedlet_data, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    for (uint16_t i = 0; i < BEDLET_COUNT; ++i) {
        metric_record_custom(
            &metric_states,
            ",n=%d v=%u",
            i,
            static_cast<unsigned>(bedlet_data.value.fault_status[i]));
        metric_record_custom(
            &metric_temps,
            ",n=%d v=%.2f",
            i,
            static_cast<double>(bedlet_data.value.measured_temperature[i]) / MODBUS_TEMPERATURE_REGISTERS_SCALE);
        metric_record_custom(
            &metric_pwms,
            ",n=%d v=%.2f",
            i,
            static_cast<double>(bedlet_data.value.pwm_state[i]));
        metric_record_custom(
            &metric_regulators,
            ",n=%d p=%d,i=%d,d=%d,tc=%d",
            i,
            bedlet_data.value.p_value[i],
            bedlet_data.value.i_value[i],
            bedlet_data.value.d_value[i],
            bedlet_data.value.tc_value[i]);

        const auto fault_int { ftrstd::to_underlying(bedlet_data.value.fault_status[i]) };
        if (fault_int > 0) {
            const auto bedlet_number { bedlet_idx_to_board_number(i) };
            if constexpr (option::is_knoblet) {
                log_debug(ModularBed, "Bedlet %d: Error %d", bedlet_number, fault_int);
                break;
            }

            if (fault_int & ftrstd::to_underlying(HeatbedletError::HeaterDisconnected)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_HEATER_DISCONNECTED, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::HeaterShortCircuit)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_HEATER_SHORT, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::TemperatureBelowMinimum)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_MINTEMP_ERR, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::TemperatureAboveMaximum)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_MAXTEMP_ERR, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::TemperatureDropDetected)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_DROP_TEMP, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::TemperaturePeakDetected)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_PEAK_TEMP, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::PreheatError)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_PREHEAT_ERR, bedlet_number);
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::TestHeatingError)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_TEST_HEATING_ERR, bedlet_number);
#if PRINTER_IS_PRUSA_iX()
            } else if (fault_int & ftrstd::to_underlying(HeatbedletError::HeaterConnected)) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MB_HEATER_CONNECTED, bedlet_number);
#endif
            } else {
                fatal_error(ErrCode::ERR_SYSTEM_MB_UNKNOWN_ERR, bedlet_number, fault_int);
            }
        }
    }
    return status;
}

CommunicationStatus ModularBed::read_general_fault() {
    CommunicationStatus status = bus.read(unit, general_fault, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    log_debug(ModularBed, "Fault status: %d", static_cast<int>(general_fault.value));
    metric_record_integer(&metric_state, static_cast<int>(general_fault.value));

    const auto fault_int { ftrstd::to_underlying(general_fault.value) };
    if (fault_int & ftrstd::to_underlying(SystemError::OverCurrent)) {
        fatal_error(ErrCode::ERR_ELECTRO_MB_OVERCURRENT);
    } else if (fault_int & ftrstd::to_underlying(SystemError::UnexpectedCurrent)) {
        fatal_error(ErrCode::ERR_ELECTRO_MB_INVALID_CURRENT);
    } // other errors are handled by heatbedlet
    return status;
}

CommunicationStatus ModularBed::write_bedlet_target_temp() {
    CommunicationStatus status = bus.write(unit, bedlet_target_temp);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    for (uint16_t i = 0; i < BEDLET_COUNT; ++i) {
        metric_record_custom(
            &metric_targets,
            ",n=%d v=%.2f",
            i,
            static_cast<double>(bedlet_target_temp.value[i]) / MODBUS_TEMPERATURE_REGISTERS_SCALE);
    }
    return status;
}

CommunicationStatus ModularBed::read_bedlet_measured_max_current() {
    CommunicationStatus status = bus.read(unit, bedlet_measured_max_current, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    for (uint16_t i = 0; i < BEDLET_COUNT; ++i) {
        metric_record_custom(
            &metric_bedlet_currents,
            ",n=%d v=%.2f",
            i,
            static_cast<double>(bedlet_measured_max_current.value[i]) / MODBUS_CURRENT_REGISTERS_SCALE);
    }
    return status;
}

CommunicationStatus ModularBed::read_mcu_temperature() {
    CommunicationStatus status = bus.read(unit, mcu_temperature, MAX_UNREAD_MS);
    if (status != CommunicationStatus::OK) {
        return status;
    }

    log_debug(ModularBed, "MCU Temperature: %d", mcu_temperature.value);
    metric_record_float(&metric_mcu_temperature, mcu_temperature.value);
    sensor_data().mbedMCUTemperature = mcu_temperature.value;
    return status;
}

void ModularBed::clear_fault() {
    Lock guard(mutex);
    clear_fault_status.value = true;
    clear_fault_status.dirty = true;
}

float ModularBed::get_temp(const uint16_t idx) {
    Lock guard(mutex);
    return static_cast<float>(bedlet_data.value.measured_temperature[idx]) / MODBUS_TEMPERATURE_REGISTERS_SCALE;
}

void ModularBed::set_print_fan_active(bool value) {
    Lock guard(mutex);
    print_fan_active.dirty = true;
    print_fan_active.value = value;
}

float ModularBed::get_temp(const uint8_t column, const uint8_t row) {
    // Private, not locked.
    return get_temp(idx(column, row));
}

void ModularBed::set_target(const uint8_t column, const uint8_t row, const float temp) {
    Lock guard(mutex);
    set_target(idx(column, row), temp);
}

void ModularBed::set_target(const uint8_t idx, const float temp) {
    // Private, not locked
    bedlet_target_temp.value[idx] = temp * MODBUS_TEMPERATURE_REGISTERS_SCALE;
    bedlet_target_temp.dirty = true;
}

float ModularBed::get_target(const uint8_t idx) {
    // Private, not locked
    return static_cast<float>(bedlet_target_temp.value[idx]) / MODBUS_TEMPERATURE_REGISTERS_SCALE;
}

float ModularBed::get_target(const uint8_t column, const uint8_t row) {
    Lock guard(mutex);
    return get_target(idx(column, row));
}

uint16_t ModularBed::idx(const uint8_t column, const uint8_t row) {
    assert(column < BEDLET_MAX_X);
    assert(row < BEDLET_MAX_Y);

#if PRINTER_IS_PRUSA_XL()
    static_assert(BEDLET_MAX_X == 4);
    static_assert(BEDLET_MAX_Y == 4);
    static constexpr uint8_t map[BEDLET_MAX_Y][BEDLET_MAX_X] = {
        { 7, 8, 9, 10 },
        { 6, 5, 12, 11 },
        { 3, 4, 13, 14 },
        { 2, 1, 16, 15 },
    };
#elif PRINTER_IS_PRUSA_iX()
    static_assert(BEDLET_MAX_X == 3);
    static_assert(BEDLET_MAX_Y == 3);
    static constexpr uint8_t map[BEDLET_MAX_Y][BEDLET_MAX_X] = {
        { 5, 6, 11 },
        { 4, 13, 12 },
        { 3, 2, 14 },
    };
#else
    #error "Not defined for this printer."
#endif
    return map[row][column] - 1;
}

ModularBed::cost_and_enable_mask_t ModularBed::touch_side(uint16_t enabled_mask, side_t side) {
    uint8_t min_cost = std::numeric_limits<uint8_t>::max();
    uint16_t min_to_enable = 0;

    for (uint8_t x = 0; x < BEDLET_MAX_X; ++x) {
        for (uint8_t y = 0; y < BEDLET_MAX_Y; ++y) {

            // if this bedlet is not enabled, don't calculate how to touch side from here
            if ((enabled_mask & (1 << idx(x, y))) == 0) {
                continue;
            }

            uint8_t cost = 0;
            uint16_t to_enable = 0;

            // loop towards selected side, calculate cost while doing so
            int8_t i = side.dim == 0 ? x : y;
            uint8_t i_max = side.dim == 0 ? BEDLET_MAX_X : BEDLET_MAX_Y;
            while (1) {
                i += side.sign;
                if (i >= 0 && i < i_max) {
                    ++cost;
                    to_enable |= 1 << idx(side.dim == 0 ? i : x,
                                     side.dim == 1 ? i : y);
                } else {
                    break;
                }
            }

            if (cost < min_cost) {
                min_cost = cost;
                min_to_enable = to_enable;
            }
        }
    }
    return cost_and_enable_mask_t { min_cost, min_to_enable };
}

void ModularBed::update_bedlet_temps(uint16_t enabled_mask, float target_temp) {
    Lock guard(mutex);
    // first calculate what bedlets to enable so that we heat towards two sides
    // this avoid bed warping, because when heated towards two sides, it can expand without making mountain in the middle
    if (expand_to_sides_enabled) {
        enabled_mask = expand_to_sides(enabled_mask, target_temp);
    }

    // now update gradient so that temperature is decreased gradually, and neighbouring bedlets help with heatup
    update_gradients(enabled_mask);
}

uint16_t ModularBed::expand_to_sides(uint16_t enabled_mask, float target_temp) {
    // calculate costs of touching all different sides
    cost_and_enable_mask_t cost_and_masks[4];
    cost_and_masks[0] = touch_side(enabled_mask, side_t { 0, 1 }); // right
    cost_and_masks[1] = touch_side(enabled_mask, side_t { 0, -1 }); // left
    cost_and_masks[2] = touch_side(enabled_mask, side_t { 1, 1 }); // up
    cost_and_masks[3] = touch_side(enabled_mask, side_t { 1, -1 }); // down

    // order sides by ascending costs
    std::ranges::sort(cost_and_masks, [](cost_and_enable_mask_t a, cost_and_enable_mask_t b) { return a.cost < b.cost; });

    // now touch at least two sides (if side is already touching, enable mask is zero so it won't do anything)
    uint16_t side_expand_mask = cost_and_masks[0].enable_mask | cost_and_masks[1].enable_mask;

    // apply new enable mask, update temperature
    enabled_mask |= side_expand_mask;
    for (uint8_t i = 0; i < BEDLET_COUNT; i++) {
        if ((side_expand_mask & (1 << i))) {
            set_target(i, target_temp);
        }
    }

    return enabled_mask;
}

void ModularBed::update_gradients(uint16_t enabled_mask) {
    // first reset target of not enabled bedlets to zero
    for (uint8_t i = 0; i < BEDLET_COUNT; i++) {
        if ((enabled_mask & (1 << i)) == 0) {
            bedlet_target_temp.value[i] = 0;
            bedlet_target_temp.dirty = true;
        }
    }

    // now compute new target temperature
    for (uint8_t x1 = 0; x1 < BEDLET_MAX_X; ++x1) {
        for (uint8_t y1 = 0; y1 < BEDLET_MAX_Y; ++y1) {
            const uint16_t idx1 = idx(x1, y1);
            const uint16_t temp1 = bedlet_target_temp.value[idx1];
            if ((enabled_mask & (1 << idx1)) == 0) { // if this bedlet is not enabled, don't calculate gradient from it
                continue;
            }

            for (uint8_t x2 = 0; x2 < BEDLET_MAX_X; ++x2) {
                for (uint8_t y2 = 0; y2 < BEDLET_MAX_Y; ++y2) {
                    const uint16_t idx2 = idx(x2, y2);
                    if ((enabled_mask & (1 << idx2))) { // if this bedlet is enabled, don't change it's temperature
                        continue;
                    }

                    const float dist = std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2)); // distance between bedlets
                    if (dist > bedlet_gradient_cutoff) {
                        continue; // if bedlet distance is over BEDLET_GRADIENT_CUTOFF, don't do anything, temperature is already zero
                    }

                    const int16_t temp2 = temp1 - temp1 * pow(1 / bedlet_gradient_cutoff * dist, bedlet_gradient_exponent);
                    bedlet_target_temp.value[idx2] = std::max(temp2, (int16_t)bedlet_target_temp.value[idx2]);
                    bedlet_target_temp.dirty = true;
                }
            }
        }
    }
}

float ModularBed::get_heater_current() {
    Lock guard(mutex);
    return (currents.value.A_measured + currents.value.B_measured) / 1000.0;
}

uint16_t ModularBed::get_mcu_temperature() {
    Lock guard(mutex);
    return mcu_temperature.value;
}

ModularBed modular_bed(puppyModbus, PuppyBootstrap::get_modbus_address_for_dock(Dock::MODULAR_BED));
} // namespace buddy::puppies

AdvancedModularBed *const advanced_modular_bed = &buddy::puppies::modular_bed;
