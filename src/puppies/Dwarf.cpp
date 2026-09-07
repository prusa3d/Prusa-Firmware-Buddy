#include <limits>

#include <puppies/Dwarf.hpp>
#include <fifo_coder/fifo_decoder.hpp>
#include <freertos/mutex.hpp>

#include "bsod.h"
#include <logging/log.hpp>
#include "loadcell.hpp"
#include "timing.h"
#include <logging/log_dest_bufflog.hpp>
#include "metric.h"
#include <puppies/PuppyBootstrap.hpp>
#include <i18n.h>
#include "Marlin/src/inc/MarlinConfig.h"
#include "utility_extensions.hpp"
#include "dwarf_errors.hpp"
#include "otp.hpp"
#include "adc.hpp"
#include <config_store/store_instance.hpp>
#include "Marlin/src/module/prusa/accelerometer.h"
#include <common/power_panic.hpp>
#include <common/printer_model.hpp>
#include <common/extended_printer_type.hpp>

using namespace fifo_coder;

namespace buddy::puppies {

using Lock = std::unique_lock<freertos::Mutex>;

LOG_COMPONENT_DEF(Dwarf_1, logging::Severity::info);
LOG_COMPONENT_DEF(Dwarf_2, logging::Severity::info);
LOG_COMPONENT_DEF(Dwarf_3, logging::Severity::info);
LOG_COMPONENT_DEF(Dwarf_4, logging::Severity::info);
LOG_COMPONENT_DEF(Dwarf_5, logging::Severity::info);
LOG_COMPONENT_DEF(Dwarf_6, logging::Severity::info);

/// Shorthand macro to send log message to proper log component, depending on which dwarf instance its called on
/// NOTE: has to ba called inside member funciton of Dwarf class
#define DWARF_LOG(severity, fmt, ...) _log_event(severity, &this->log_component, fmt, ##__VA_ARGS__);

METRIC_DEF(metric_fast_refresh_delay, "dwarf_fast_refresh_delay", METRIC_VALUE_INTEGER, 0, METRIC_DISABLED);

METRIC_DEF(metric_dwarf_picked_raw, "dwarf_picked_raw", METRIC_VALUE_CUSTOM, 100, METRIC_DISABLED);
METRIC_DEF(metric_dwarf_parked_raw, "dwarf_parked_raw", METRIC_VALUE_CUSTOM, 100, METRIC_DISABLED);

METRIC_DEF(metric_dwarf_heater_current, "dwarf_heat_curr", METRIC_VALUE_CUSTOM, 100, METRIC_DISABLED);
METRIC_DEF(metric_dwarf_heater_pwm, "dwarf_heat_pwm", METRIC_VALUE_CUSTOM, 100, METRIC_DISABLED);

Dwarf::Dwarf(const uint8_t dwarf_number, uint8_t modbus_address)
    : ModbusDevice(modbus_address)
    , mutex(new freertos::Mutex)
    , dwarf_number(dwarf_number)
    , log_component(get_log_component(dwarf_number))
    , selected(false)
    , time_sync(dwarf_number + 1)
    , loadcell_samplerate {} {

    // Atomic mirrors initial values — must not trigger min-temp faults.
    hotend_temp.store(HEATER_0_MINTEMP + 1);
    heatbreak_temp.store(HEATBREAK_MINTEMP + 1);
    heatbreak_target_temp_desired.store(DEFAULT_HEATBREAK_TEMPERATURE);

    set_cheese_led(); // Set LED by eeprom config
    set_status_led(); // Default status LED mode
}

CommunicationStatus Dwarf::refresh(PuppyModbus &bus) {
    Lock guard(*mutex);
    typedef CommunicationStatus (Dwarf::*MethodType)(PuppyModbus &);
    static constexpr MethodType funcs[] = {
        &Dwarf::read_general_status,
        &Dwarf::read_discrete_general_status,
        &Dwarf::write_general,
        &Dwarf::write_tmc_enable,
        &Dwarf::run_time_sync,
    };
    if (++refresh_nr >= std::size(funcs)) {
        refresh_nr = 0;
    }
    return (this->*funcs[refresh_nr])(bus);
}

CommunicationStatus Dwarf::read_discrete_general_status(PuppyModbus &bus) {
    ModbusDiscreteInputBlock<GENERAL_DISCRETE_INPUTS_ADDR, DiscreteGeneralStatus_t> block {};
    block.last_read_timestamp_ms = discrete_general_status_last_read_ms;
    const CommunicationStatus status = bus.read(unit, block, 250);
    discrete_general_status_last_read_ms = block.last_read_timestamp_ms;
    if (status == CommunicationStatus::OK) {
        uint8_t disc = 0;
        if (block.value.is_picked) {
            disc |= DISC_PICKED;
        }
        if (block.value.is_parked) {
            disc |= DISC_PARKED;
        }
        if (block.value.is_button_up_pressed) {
            disc |= DISC_BTN_UP;
        }
        if (block.value.is_button_down_pressed) {
            disc |= DISC_BTN_DN;
        }
        discrete_status.store(disc);

        DWARF_LOG(logging::Severity::debug, "Is parked: %d", block.value.is_parked);
        DWARF_LOG(logging::Severity::debug, "Is picked: %d", block.value.is_picked);
    }
    return status;
}

CommunicationStatus Dwarf::read_general_status(PuppyModbus &bus) {
    ModbusInputRegisterBlock<FAULT_STATUS_ADDR, RegisterGeneralStatus_t> block {};
    block.last_read_timestamp_ms = register_general_status_last_read_ms;
    const CommunicationStatus status = bus.read(unit, block, 250);
    register_general_status_last_read_ms = block.last_read_timestamp_ms;
    if (status == CommunicationStatus::OK) {
        if (block.value.FaultStatus != dwarf_shared::errors::FaultStatusMask::NO_FAULT) {
            handle_dwarf_fault(bus, block.value.FaultStatus);
        }

        // Publish atomic mirrors for lock-free access from Marlin.
        tool_filament_sensor.store(block.value.ToolFilamentSensor);
        hotend_temp.store(static_cast<int16_t>(block.value.HotendMeasuredTemperature));
        heater_pwm.store(block.value.HotendPWMState);
        heatbreak_temp.store(static_cast<int16_t>(block.value.HeatBreakMeasuredTemperature));
        for (size_t i = 0; i < NUM_FANS; ++i) {
            fan_pwm[i].store(block.value.fan[i].pwm);
            fan_rpm[i].store(block.value.fan[i].rpm);
        }
        for (size_t i = 0; i < NUM_FANS; ++i) {
            fan_state[i].store(static_cast<uint8_t>(block.value.fan[i].state));
        }

        uint8_t rpm_ok_mask = 0;
        for (size_t i = 0; i < NUM_FANS; ++i) {
            if (block.value.fan[i].is_rpm_ok) {
                rpm_ok_mask |= static_cast<uint8_t>(1u << i);
            }
        }
        fan_rpm_ok.store(rpm_ok_mask);
        mcu_temperature.store(static_cast<int16_t>(block.value.MCUTemperature));
        board_temperature.store(static_cast<int16_t>(block.value.BoardTemperature));
        v24_mV.store(block.value.system_24V_mV);
        heater_current_mA.store(block.value.heater_current_mA);

        metric_record_custom(&metric_dwarf_parked_raw, ",n=%u v=%ii", dwarf_number + 1, block.value.IsParkedRaw);
        metric_record_custom(&metric_dwarf_picked_raw, ",n=%u v=%ii", dwarf_number + 1, block.value.IsPickedRaw);
        metric_record_custom(&metric_dwarf_heater_current, ",n=%u v=%d", dwarf_number + 1, block.value.heater_current_mA);
        metric_record_custom(&metric_dwarf_heater_pwm, ",n=%u v=%d", dwarf_number + 1, block.value.HotendPWMState);
    }
    return status;
}

CommunicationStatus Dwarf::ping(PuppyModbus &bus) {
    Lock guard(*mutex);
    return bus.read(unit, GeneralStatic);
}

CommunicationStatus Dwarf::initial_scan(PuppyModbus &bus) {
    Lock guard(*mutex);
    time_sync.init();
    run_time_sync(bus);

    // Update static values
    CommunicationStatus status = bus.read(unit, GeneralStatic);
    if (status == CommunicationStatus::ERROR) {
        return status;
    }

    DWARF_LOG(logging::Severity::info, "HwBomId: %d", GeneralStatic.value.HwBomId);
    DWARF_LOG(logging::Severity::info, "HwOtpTimestsamp: %" PRIu32, GeneralStatic.value.HwOtpTimestsamp);

    serial_nr_t sn = {}; // Last byte has to be '\0'
    static constexpr uint16_t raw_datamatrix_regsize = std::to_underlying(SystemInputRegister::hw_raw_datamatrix_last)
        - std::to_underlying(SystemInputRegister::hw_raw_datamatrix_first) + 1;
    // Check size of text -1 as the terminating \0 is not sent
    static_assert((raw_datamatrix_regsize * sizeof(uint16_t)) == (sn.size() - 1), "Size of raw datamatrix doesn't fit modbus registers");

    for (uint16_t i = 0; i < raw_datamatrix_regsize; ++i) {
        sn[i * 2] = GeneralStatic.value.HwDatamatrix[i] & 0xff;
        sn[i * 2 + 1] = GeneralStatic.value.HwDatamatrix[i] >> 8;
    }
    DWARF_LOG(logging::Severity::info, "HwDatamatrix: %s", sn.data());

    // read discrete general stats - contains data about picked/parked, and that is needed immediately upon init to pick correct tool
    status = read_discrete_general_status(bus);

    fan_mode = (PrinterModelInfo::current().model == PrinterModel::xls) ? FanMode::XLS_NATIVE : FanMode::XL_LEGACY;

    general_write_dirty.store(true);
    TmcEnable.dirty = true;
    IsSelectedCoil.dirty = true;
    LoadcellEnableCoil.dirty = true;
    AccelerometerEnableCoil.dirty = true;
    selected = false;

    // !!! MUST be after resetting all the stuff to avoid race conditions
    // The intention is that when someone detects a reset, we want to guarantee that the data is already marked as invalid
    // and will become valid again only after it is properly fetched from the newly reset puppy.
    reset_counter++;

    // Write coil values that are not written automatically
    if (bus.write(unit, IsSelectedCoil) == CommunicationStatus::ERROR) {
        return CommunicationStatus::ERROR;
    }
    if (bus.write(unit, LoadcellEnableCoil) == CommunicationStatus::ERROR) {
        return CommunicationStatus::ERROR;
    }
    if (bus.write(unit, AccelerometerEnableCoil) == CommunicationStatus::ERROR) {
        return CommunicationStatus::ERROR;
    }

    return status;
}

bool Dwarf::dispatch_log_event() {
    // Look for EOT byte - end of log entry
    size_t eot_pos = 0;
    while (eot_pos < log_line_pos && log_line_buffer[eot_pos] != BUFFLOG_TERMINATION_CHAR) {
        eot_pos++;
    }

    if (eot_pos == log_line_pos) {
        return false;
    }

    // Log event
    if (eot_pos) {
        DWARF_LOG(logging::Severity::info, "%.*s", eot_pos, log_line_buffer.data());
    }

    // Compact buffer
    log_line_pos -= eot_pos + 1;
    memmove(log_line_buffer.data(), &log_line_buffer[eot_pos + 1], log_line_pos);

    return true;
}

CommunicationStatus Dwarf::fifo_refresh(PuppyModbus &bus, uint32_t cycle_ticks_ms) {
    Lock guard(*mutex);
    // pull fifo every 200 ms
    if (last_pull_ms + DWARF_FIFO_PULL_PERIOD > cycle_ticks_ms) {
        return CommunicationStatus::SKIPPED;
    }

    bool more;
    CommunicationStatus status = pull_fifo_nolock(bus, more);
    if (!more && status == CommunicationStatus::OK) {
        last_pull_ms = cycle_ticks_ms; // Wait before next pull only if all is read
    }
    return status;
}

CommunicationStatus Dwarf::pull_fifo(PuppyModbus &bus, bool &more) {
    Lock guard(*mutex);
    return pull_fifo_nolock(bus, more);
}

CommunicationStatus Dwarf::pull_fifo_nolock(PuppyModbus &bus, bool &more) {
    // Read coded FIFO
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo;
    size_t read = 0;
    CommunicationStatus status = bus.ReadFIFO(unit, ENCODED_FIFO_ADDRESS, fifo, read, FIFO_RETRIES);
    if (status == CommunicationStatus::ERROR) {
        more = true; // Request failed, most probably there is more data waiting
        PrusaAccelerometer::set_possible_overflow();
        return status;
    }

    // calculate metric of read latency
    static uint32_t time_last_read = 0;
    auto now = ticks_ms();
    metric_record_integer(&metric_fast_refresh_delay, now - time_last_read);
    time_last_read = now;

    if (!read) {
        more = false;
        return CommunicationStatus::OK;
    }

    Decoder decoder(fifo, read);
    decoder.decode(*this);

    // Update sampling rate of the loadcell.ProcessSample()
    if (loadcell_samplerate.count > 30) {
        float interval = static_cast<float>(loadcell_samplerate.last_timestamp - loadcell_samplerate.last_processed_timestamp) / static_cast<float>(1000 * loadcell_samplerate.count);
        // Ignore invalid values, values outside of 25% of expected value may be caused by glitch in modbus communication
        if (interval >= loadcell_samplerate.expected * 0.75f && interval <= loadcell_samplerate.expected * 1.25f) {
            loadcell.analysis.SetSamplingIntervalMs(interval); // Update sampling interval
        }
        loadcell_samplerate.count = 0;
        loadcell_samplerate.last_processed_timestamp = loadcell_samplerate.last_timestamp;
    }

    more = decoder.more();
    return status;
}

CommunicationStatus Dwarf::write_general(PuppyModbus &bus) {
    // Clear dirty before snapshotting; a racing setter then either lands in
    // our snapshot or re-marks dirty for the next cycle.
    const bool was_dirty = general_write_dirty.exchange(false);

    ModbusHoldingRegisterBlock<GENERAL_WRITE_REQUEST, GeneralWrite_t> block {};
    block.value.HotendRequestedTemperature = hotend_target_temp_desired.load();
    block.value.HeatbreakRequestedTemperature = heatbreak_target_temp_desired.load();
    for (size_t i = 0; i < NUM_FANS; i++) {
        block.value.fan_pwm[i] = fan_pwm_desired[i].load();
    }
    block.value.led_pwm.not_selected = led_pwm.not_selected;
    block.value.led_pwm.selected = led_pwm.selected;
    for (size_t i = 0; i < dwarf_shared::StatusLed::REG_SIZE; i++) {
        block.value.status_led[i] = status_led[i];
    }
    block.value.pid.p = pid.p;
    block.value.pid.i = pid.i;
    block.value.pid.d = pid.d;
    block.value.fan_mode = static_cast<uint16_t>(fan_mode);
    block.dirty = was_dirty;

    const CommunicationStatus status = bus.write(unit, block);
    if (status == CommunicationStatus::ERROR && was_dirty) {
        // Write didn't go through, keep work for next cycle.
        general_write_dirty.store(true);
    }

    if (status == CommunicationStatus::OK) {
        DWARF_LOG(logging::Severity::debug, "Written GeneralWrite");
    }
    return status;
}

CommunicationStatus Dwarf::write_tmc_enable(PuppyModbus &bus) {
    CommunicationStatus status = bus.write(unit, TmcEnable);
    if (status == CommunicationStatus::ERROR) {
        return status;
    }

    DWARF_LOG(logging::Severity::debug, "Written TmcEnable");
    return status;
}

uint32_t Dwarf::tmc_read(PuppyModbus &bus, uint8_t addressByte) {
    Lock guard(*mutex);

    TmcReadRequest.value.address = addressByte;
    TmcReadRequest.dirty = true;
    if (bus.write(unit, TmcReadRequest) != CommunicationStatus::ERROR) {
        if (bus.read(unit, TmcReadResponse) != CommunicationStatus::ERROR) {
            DWARF_LOG(logging::Severity::debug, "TMC on dwarf read (%d:%" PRIu32 ")",
                addressByte, TmcReadResponse.value.value);
            return TmcReadResponse.value.value;
        } else {
            DWARF_LOG(logging::Severity::error, "TMC read response FAIL");
        }
    } else {
        DWARF_LOG(logging::Severity::error, "TMC read request FAIL");
    }
    // todo: what to do in case of error?
    return 0;
}

void Dwarf::tmc_write(PuppyModbus &bus, uint8_t addressByte, uint32_t config) {
    Lock guard(*mutex);

    TmcWriteRequest.value.address = addressByte;
    TmcWriteRequest.value.data = config;
    TmcWriteRequest.dirty = true;

    if (bus.write(unit, TmcWriteRequest) != CommunicationStatus::ERROR) {
        DWARF_LOG(logging::Severity::debug, "Write to TMC dwarf success (%d:%" PRIu32 ")",
            addressByte, config);
    } else {
        DWARF_LOG(logging::Severity::error, "Write to TMC dwarf FAIL");
    }
}

void Dwarf::tmc_set_enable(PuppyModbus &bus, bool state) {
    Lock guard(*mutex);

    uint16_t new_state = state ? 1 : 0;
    if (TmcEnable.value == new_state) {
        return;
    }

    TmcEnable.value = new_state;
    TmcEnable.dirty = true;
    auto result = write_tmc_enable(bus);
    if (result != CommunicationStatus::OK) {
        DWARF_LOG(logging::Severity::critical, "Enable pin write error");
    }
}

bool Dwarf::is_tmc_enabled() {
    Lock guard(*mutex);

    return TmcEnable.value;
}

float Dwarf::get_hotend_temp() {
    // Modbus carries the temperature as uint16_t but it encodes a signed int16_t value.
    return static_cast<float>(static_cast<int16_t>(hotend_temp.load()));
}

CommunicationStatus Dwarf::set_hotend_target_temp(float target) {
    const uint16_t value = static_cast<uint16_t>(target);
    if (hotend_target_temp_desired.exchange(value) != value) {
        general_write_dirty.store(true);
    }
    return CommunicationStatus::OK;
}

int Dwarf::get_heater_pwm() {
    return static_cast<int>(heater_pwm.load());
}

bool Dwarf::is_picked() const {
    return (discrete_status.load() & DISC_PICKED) != 0;
}

bool Dwarf::is_parked() const {
    return (discrete_status.load() & DISC_PARKED) != 0;
}

bool Dwarf::is_button_up_pressed() const {
    return (discrete_status.load() & DISC_BTN_UP) != 0;
}

bool Dwarf::is_button_down_pressed() const {
    return (discrete_status.load() & DISC_BTN_DN) != 0;
}

CommunicationStatus Dwarf::run_time_sync(PuppyModbus &bus) {
    RequestTiming timing;
    CommunicationStatus status = bus.read(unit, TimeSync, 1000, &timing);
    if (status == CommunicationStatus::ERROR) {
        DWARF_LOG(logging::Severity::error, "Failed to read fault status register");
        return status;
    }

    if (status != CommunicationStatus::SKIPPED) {
        time_sync.sync(TimeSync.value.dwarf_time_us, timing);
    }

    return status;
}

[[nodiscard]] bool Dwarf::is_selected() const {
    Lock guard(*mutex);
    return selected;
}

CommunicationStatus Dwarf::set_selected(PuppyModbus &bus, bool selected) {
    Lock guard(*mutex);

    IsSelectedCoil.dirty = true;
    IsSelectedCoil.value = selected;
    if (bus.write(unit, IsSelectedCoil) != CommunicationStatus::OK) {
        return CommunicationStatus::ERROR;
    }

    this->selected = selected;

    if (selected) {
        // Enable loadcell for dwarf being selected in case the accelerometer is not already enabled
        // This condition prevents replacing accelerometer with loadcell when recovering from puppy failure
        if (!AccelerometerEnableCoil.value) {
            if (!set_loadcell_nolock(bus, true)) {
                return CommunicationStatus::ERROR;
            }
        }
    } else {
        // Disable accelerometer and loadcell for dwarf being unselected
        if (!set_loadcell_nolock(bus, false) || !set_accelerometer_nolock(bus, false)) {
            return CommunicationStatus::ERROR;
        }
    }

    return CommunicationStatus::OK;
}

bool Dwarf::set_accelerometer(PuppyModbus &bus, bool active) {
    Lock guard(*mutex);

    return set_accelerometer_nolock(bus, active);
}

bool Dwarf::set_accelerometer_nolock(PuppyModbus &bus, bool active) {
    if (active && !this->selected) {
        return false;
    }

    return raw_set_loadcell(bus, !active && this->selected) && raw_set_accelerometer(bus, active);
}

bool Dwarf::set_loadcell(PuppyModbus &bus, bool active) {
    Lock guard(*mutex);

    return set_loadcell_nolock(bus, active);
}

bool Dwarf::set_loadcell_nolock(PuppyModbus &bus, bool active) {
    if (active && !this->selected) {
        return false;
    }

    if (active) {
        return raw_set_accelerometer(bus, false) && raw_set_loadcell(bus, true);
    }

    return raw_set_loadcell(bus, false);
}

bool Dwarf::raw_set_loadcell(PuppyModbus &bus, bool enable) {
    LoadcellEnableCoil.dirty = true;
    LoadcellEnableCoil.value = enable;
    return bus.write(unit, LoadcellEnableCoil) == CommunicationStatus::OK;
}

bool Dwarf::raw_set_accelerometer(PuppyModbus &bus, bool enable) {
    AccelerometerEnableCoil.dirty = true;
    AccelerometerEnableCoil.value = enable;
    return bus.write(unit, AccelerometerEnableCoil) == CommunicationStatus::OK;
}

constexpr logging::Component &Dwarf::get_log_component(uint8_t dwarf_number) {
    switch (dwarf_number) {
    case 0:
        return __log_component_Dwarf_1;
    case 1:
        return __log_component_Dwarf_2;
    case 2:
        return __log_component_Dwarf_3;
    case 3:
        return __log_component_Dwarf_4;
    case 4:
        return __log_component_Dwarf_5;
    case 5:
        return __log_component_Dwarf_6;
    default:
        bsod("Unknown");
    }
}

IFSensor::value_type Dwarf::get_tool_filament_sensor() {
    // ensure AdcGet::undefined_value is representable within FSensor::value_type
    static_assert(static_cast<IFSensor::value_type>(AdcGet::undefined_value) == AdcGet::undefined_value);

    // widen the type to match the HX717 data type and translate the undefined value for consistency
    // Called from an interrupt, therefore we don't lock, but use cached value in an atomic.
    IFSensor::value_type value = tool_filament_sensor.load();
    if (value == AdcGet::undefined_value) {
        value = IFSensor::undefined_value;
    }
    return value;
}

int16_t Dwarf::get_mcu_temperature() {
    return mcu_temperature.load();
}

int16_t Dwarf::get_board_temperature() {
    return board_temperature.load();
}

float Dwarf::get_24V() {
    return v24_mV.load() / 1000.0f;
}

float Dwarf::get_heater_current() {
    return heater_current_mA.load() / 1000.0f;
}

void Dwarf::set_heatbreak_target_temp(int16_t target) {
    const uint16_t value = static_cast<uint16_t>(target);
    if (heatbreak_target_temp_desired.exchange(value) != value) {
        general_write_dirty.store(true);
    }
}

void Dwarf::set_fan(uint8_t fan, uint16_t target) {
    debug_assert(fan < NUM_FANS);
    if (fan_pwm_desired[fan].exchange(target) != target) {
        general_write_dirty.store(true);
    }
}

void Dwarf::set_fan_auto(uint8_t fan) {
    debug_assert(fan < NUM_FANS);
    if (fan_pwm_desired[fan].exchange(FAN_MODE_AUTO_PWM) != FAN_MODE_AUTO_PWM) {
        general_write_dirty.store(true);
    }
}

void Dwarf::set_fan_mode(Dwarf::FanMode mode) {
    Lock guard(*mutex);
    if (mode != fan_mode) {
        fan_mode = mode;
        general_write_dirty.store(true);
    }
}

void Dwarf::set_cheese_led(uint8_t pwr_selected, uint8_t pwr_not_selected) {
    Lock guard(*mutex);

    led_pwm.selected = pwr_selected;
    led_pwm.not_selected = pwr_not_selected;
    general_write_dirty.store(true);
}

void Dwarf::set_cheese_led() {
    set_cheese_led(config_store().tool_leds_enabled.get() ? 0xff : 0x00, 0x00);
}

void Dwarf::set_status_led(dwarf_shared::StatusLed::Mode mode, uint8_t r, uint8_t g, uint8_t b) {
    Lock guard(*mutex);

    dwarf_shared::StatusLed encoded(mode, r, g, b);
    for (size_t i = 0; i < dwarf_shared::StatusLed::REG_SIZE; i++) {
        status_led[i] = encoded.get_reg_value(i);
    }
    general_write_dirty.store(true);
}

void Dwarf::set_pid(float p, float i, float d) {
    Lock guard(*mutex);

    pid.p = p;
    pid.i = i;
    pid.d = d;
    general_write_dirty.store(true);
}

void Dwarf::handle_dwarf_fault(PuppyModbus &bus, dwarf_shared::errors::FaultStatusMask fault_status) {
    debug_assert(fault_status != dwarf_shared::errors::FaultStatusMask::NO_FAULT);

    const auto fault_int { std::to_underlying(fault_status) };
    DWARF_LOG(logging::Severity::error, "Fault status: %d", fault_int);

    if (fault_int & std::to_underlying(dwarf_shared::errors::FaultStatusMask::MARLIN_KILLED)) {
        // read error string from dwarf
        std::span<char> title_span(reinterpret_cast<char *>(&MarlinErrorString.value.title[0]), sizeof(MarlinErrorString.value.title));
        std::span<char> message_span(reinterpret_cast<char *>(&MarlinErrorString.value.message[0]), sizeof(MarlinErrorString.value.message));

        if (bus.read(unit, MarlinErrorString) == CommunicationStatus::ERROR) {
            // read failed, make it empty string
            title_span[0] = '\0';
            message_span[0] = '\0';
        }
        // make sure strings are zero-terminated
        title_span.back() = 0;
        message_span.back() = 0;
        DWARF_LOG(logging::Severity::error, "Dwarf %d fault %s: %s", dwarf_number + 1, title_span.data(), message_span.data());

        // Prepare module string (insert dwarf number)
        char module[31] = { 0 };
        snprintf(module, sizeof(module), "Dwarf %d: %s", dwarf_number + 1, title_span.data());

        if (int error_code; sscanf(message_span.data(), "ERRC%i", &error_code) == 1) {
            fatal_error(static_cast<ErrCode>(error_code));
        } else {
            // this calls generic fatal error
            // any marlin fault on dwarf will be decoded based on error string and converted to propper ErrCode, or displayed as-is if no error code matches
            fatal_error(message_span.data(), module);
        }

    } else if (fault_int & std::to_underlying(dwarf_shared::errors::FaultStatusMask::TMC_FAULT)) {
        fatal_error(ErrCode::ERR_SYSTEM_DWARF_TMC, dwarf_number + 1);
    } else {
        fatal_error(ErrCode::ERR_SYSTEM_DWARF_UNKNOWN_ERR, dwarf_number + 1);
    }
}

float Dwarf::get_heatbreak_temp() {
    // Modbus carries the temperature as uint16_t but it encodes a signed int16_t value.
    return static_cast<float>(static_cast<int16_t>(heatbreak_temp.load()));
}

uint16_t Dwarf::get_heatbreak_fan_pwr() {
    return fan_pwm[1].load();
}

void Dwarf::decode_log(const LogData &data) {
    // If buffer cannot handle next read, clean it
    if (log_line_pos + data.size() > log_line_buffer.size()) {
        DWARF_LOG(logging::Severity::warning, "Out of log buffer, logging incomplete data");
        DWARF_LOG(logging::Severity::info, "%.*s", log_line_pos, log_line_buffer.data());
        log_line_pos = 0;
    }

    // Copy data skipping 0 padding
    for (const char c : data) {
        if (c == 0) {
            break;
        }
        log_line_buffer[log_line_pos++] = c;
    }
    while (dispatch_log_event())
        ;
}

void Dwarf::decode_loadcell(const LoadcellRecord &data) {
    // throw away samples if time is not synced
    if (!this->time_sync.is_time_sync_valid() || !this->selected) {
        return;
    }

    // Store sample timestamp and count sample
    loadcell_samplerate.last_timestamp = this->time_sync.buddy_time_us(data.timestamp);
    loadcell_samplerate.count++;

    // Process sample
    loadcell.ProcessSample(data.loadcell_raw_value, loadcell_samplerate.last_timestamp, reset_counter.load());
}

void Dwarf::decode_accelerometer_fast(const AccelerometerFastData &data) {
    // throw away samples if not selected
    if (!this->selected) {
        return;
    }
    for (AccelerometerXyzSample sample : data) {
        PrusaAccelerometer::put_sample(sample);
    }
}

void Dwarf::decode_accelerometer_freq(const AccelerometerSamplingRate &data) {
    if (!this->selected) {
        return;
    }
    PrusaAccelerometer::set_rate(data.frequency);
}

uint16_t Dwarf::get_fan_pwm(uint8_t fan_nr) const {
    return fan_pwm[fan_nr].load();
}

uint16_t Dwarf::get_fan_rpm(uint8_t fan_nr) const {
    return fan_rpm[fan_nr].load();
}

bool Dwarf::get_fan_rpm_ok(uint8_t fan_nr) const {
    return (fan_rpm_ok.load() >> fan_nr) & 1u;
}

uint16_t Dwarf::get_fan_state(uint8_t fan_nr) const {
    return fan_state[fan_nr].load();
}

StrongIndexArray<Dwarf, DWARF_MAX_COUNT, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> dwarfs {
    Dwarf { 0, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_1) },
    Dwarf { 1, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_2) },
    Dwarf { 2, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_3) },
    Dwarf { 3, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_4) },
    Dwarf { 4, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_5) },
    Dwarf { 5, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_6) },
};

} // namespace buddy::puppies
