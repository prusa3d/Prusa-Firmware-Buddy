#include <cassert>
#include <limits>

#include "puppies/Dwarf.hpp"
#include "puppies/fifo_decoder.hpp"

#include "bsod.h"
#include "log.h"
#include "loadcell.hpp"
#include "timing.h"
#include "logging/log_dest_bufflog.h"
#include <assert.h>
#include "metric.h"
#include "puppies/PuppyBootstrap.hpp"
#include <i18n.h>
#include "Marlin/src/inc/MarlinConfig.h"
#include "utility_extensions.hpp"
#include "dwarf_errors.hpp"
#include "otp.hpp"
#include "adc.hpp"
#include <config_store/store_instance.hpp>
#include "Marlin/src/module/prusa/accelerometer.h"

using namespace common::puppies::fifo;

namespace buddy::puppies {

LOG_COMPONENT_DEF(Dwarf_1, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(Dwarf_2, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(Dwarf_3, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(Dwarf_4, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(Dwarf_5, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(Dwarf_6, LOG_SEVERITY_INFO);

/// Shorthand macro to send log message to proper log component, depending on which dwarf instance its called on
/// NOTE: has to ba called inside member funciton of Dwarf class
#define DWARF_LOG(severity, fmt, ...) _log_event(severity, &this->log_component, fmt, ##__VA_ARGS__);

METRIC_DEF(metric_fast_refresh_delay, "dwarf_fast_refresh_delay", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);

METRIC_DEF(metric_dwarf_picked_raw, "dwarf_picked_raw", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_dwarf_parked_raw, "dwarf_parked_raw", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_DISABLE_ALL);

METRIC_DEF(metric_dwarf_heater_current, "dwarf_heat_curr", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_dwarf_heater_pwm, "dwarf_heat_pwm", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_DISABLE_ALL);

Dwarf::Dwarf(PuppyModbus &bus, const uint8_t dwarf_nr, uint8_t modbus_address)
    : ModbusDevice(bus, modbus_address)
    , dwarf_nr(dwarf_nr)
    , log_component(get_log_component(dwarf_nr))
    , selected(false)
    , time_sync(this->dwarf_nr)
    , loadcell_samplerate {}
    , callbacks(Decoder::Callbacks_t {
          .log_handler = std::bind(&Dwarf::handle_log_fragment, this, std::placeholders::_1),
          .loadcell_handler = [this](LoadcellRecord data) {
              // throw away samples if time is not synced
              if (!this->time_sync.is_time_sync_valid() || !this->selected) {
                  return;
              }

              // Store sample timestamp and count sample
              loadcell_samplerate.last_timestamp = this->time_sync.buddy_time_us(data.timestamp);
              loadcell_samplerate.count++;

              // Process sample
              loadcell.ProcessSample(data.loadcell_raw_value, loadcell_samplerate.last_timestamp); },
          .accelerometer_fast_handler = [this](AccelerometerFastData data) {
              // throw away samples if not selected
              if (!this->is_selected()) {
                  return;
              }
              for (AccelerometerXyzSample sample : data) {
                  PrusaAccelerometer::put_sample(sample);
              } },
          .accelerometer_freq_handler = [this](AccelerometerSamplingRate data) {
              if (!this->is_selected()) {
                    return;
              }
              PrusaAccelerometer::set_rate(data.frequency); } }) {

    RegisterGeneralStatus.value.FaultStatus = dwarf_shared::errors::FaultStatusMask::NO_FAULT;
    RegisterGeneralStatus.value.HotendMeasuredTemperature = HEATER_0_MINTEMP + 1; // Init to temperature that won't immediately trigger mintemp
    RegisterGeneralStatus.value.HeatBreakMeasuredTemperature = HEATBREAK_MINTEMP + 1;

    RegisterGeneralStatus.value.HotendPWMState = 0;

    DiscreteGeneralStatus.value.is_picked = false;
    DiscreteGeneralStatus.value.is_parked = false;

    GeneralWrite.value.HotendRequestedTemperature = 0;
    GeneralWrite.value.HeatbreakRequestedTemperature = DEFAULT_HEATBREAK_TEMPERATURE;

    set_cheese_led(); // Set LED by eeprom config
    set_status_led(); // Default status LED mode
}

CommunicationStatus Dwarf::refresh() {
    typedef CommunicationStatus (Dwarf::*MethodType)();
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
    return (this->*funcs[refresh_nr])();
}

CommunicationStatus Dwarf::read_discrete_general_status() {
    // read general status discrete inputs
    CommunicationStatus status = bus.read(unit, DiscreteGeneralStatus, 250);
    if (status == CommunicationStatus::OK) {
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Is parked: %d", DiscreteGeneralStatus.value.is_parked);
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Is picked: %d", DiscreteGeneralStatus.value.is_picked);
    }
    return status;
}

CommunicationStatus Dwarf::read_general_status() {
    // read general status registers
    CommunicationStatus status = bus.read(unit, RegisterGeneralStatus, 250);
    if (status == CommunicationStatus::OK) {
        if (RegisterGeneralStatus.value.FaultStatus != dwarf_shared::errors::FaultStatusMask::NO_FAULT) {
            handle_dwarf_fault();
        }

        metric_record_custom(&metric_dwarf_parked_raw, ",n=%u v=%ii", dwarf_nr, RegisterGeneralStatus.value.IsParkedRaw);
        metric_record_custom(&metric_dwarf_picked_raw, ",n=%u v=%ii", dwarf_nr, RegisterGeneralStatus.value.IsPickedRaw);
        metric_record_custom(&metric_dwarf_heater_current, ",n=%u v=%d", dwarf_nr, RegisterGeneralStatus.value.heater_current_mA);
        metric_record_custom(&metric_dwarf_heater_pwm, ",n=%u v=%d", dwarf_nr, RegisterGeneralStatus.value.HotendPWMState);
    }
    return status;
}

CommunicationStatus Dwarf::ping() {
    return bus.read(unit, GeneralStatic);
}

CommunicationStatus Dwarf::initial_scan() {
    time_sync.init();
    run_time_sync();

    // Update static values
    CommunicationStatus status = bus.read(unit, GeneralStatic);
    if (status == CommunicationStatus::ERROR) {
        return status;
    }

    DWARF_LOG(LOG_SEVERITY_INFO, "HwBomId: %d", GeneralStatic.value.HwBomId);
    DWARF_LOG(LOG_SEVERITY_INFO, "HwOtpTimestsamp: %" PRIu32, GeneralStatic.value.HwOtpTimestsamp);

    serial_nr_t sn = {}; // Last byte has to be '\0'
    static constexpr uint16_t raw_datamatrix_regsize = ftrstd::to_underlying(SystemInputRegister::hw_raw_datamatrix_last)
        - ftrstd::to_underlying(SystemInputRegister::hw_raw_datamatrix_first) + 1;
    // Check size of text -1 as the terminating \0 is not sent
    static_assert((raw_datamatrix_regsize * sizeof(uint16_t)) == (sn.size() - 1), "Size of raw datamatrix doesn't fit modbus registers");

    for (uint16_t i = 0; i < raw_datamatrix_regsize; ++i) {
        sn[i * 2] = GeneralStatic.value.HwDatamatrix[i] & 0xff;
        sn[i * 2 + 1] = GeneralStatic.value.HwDatamatrix[i] >> 8;
    }
    DWARF_LOG(LOG_SEVERITY_INFO, "HwDatamatrix: %s", sn.data());

    // read discrete general stats - contains data about picked/parked, and that is needed immediately upon init to pick correct tool
    status = read_discrete_general_status();

    GeneralWrite.dirty = true;
    TmcEnable.dirty = true;
    IsSelectedCoil.dirty = true;
    LoadcellEnableCoil.dirty = true;
    AccelerometerEnableCoil.dirty = true;
    selected = false;

    // Write coil values that are not written automatically
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
        DWARF_LOG(LOG_SEVERITY_INFO, "%.*s", eot_pos, log_line_buffer.data());
    }

    // Compact buffer
    log_line_pos -= eot_pos + 1;
    memmove(log_line_buffer.data(), &log_line_buffer[eot_pos + 1], log_line_pos);

    return true;
}

CommunicationStatus Dwarf::fifo_refresh(uint32_t cycle_ticks_ms) {
    // pull fifo every 200 ms
    if (last_pull_ms + DWARF_FIFO_PULL_PERIOD > cycle_ticks_ms) {
        return CommunicationStatus::SKIPPED;
    }

    bool more;
    CommunicationStatus status = pull_fifo(more);
    if (!more && status == CommunicationStatus::OK) {
        last_pull_ms = cycle_ticks_ms; // Wait before next pull only if all is read
    }
    return status;
}

CommunicationStatus Dwarf::read_fifo(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo, size_t &read) {
    CommunicationStatus status = CommunicationStatus::SKIPPED;
    for (uint8_t i = FIFO_RETRIES; status != CommunicationStatus::OK && i != 0; i--) {
        status = bus.ReadFIFO(unit, ENCODED_FIFO_ADDRESS, fifo, read);
        if (status == CommunicationStatus::ERROR) {
            // Mark acceleration data as corrupted, but retry. Dwarf is most probably ok,
            // no need to do a full puppy reconnect.
            PrusaAccelerometer::mark_corrupted(PrusaAccelerometer::Error::corrupted_transmission_error);
        }
    }
    return status;
}

CommunicationStatus Dwarf::pull_fifo(bool &more) {
    // Read coded FIFO
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo;
    size_t read = 0;
    CommunicationStatus status = read_fifo(fifo, read);
    if (status == CommunicationStatus::ERROR) {
        more = true; // Request failed, most probably there is more data waiting
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

    decoder.decode(callbacks);

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

void Dwarf::handle_log_fragment(LogData data) {
    // If buffer cannot handle next read, clean it
    if (log_line_pos + data.size() > log_line_buffer.size()) {
        DWARF_LOG(LOG_SEVERITY_WARNING, "Out of log buffer, logging incomplete data");
        DWARF_LOG(LOG_SEVERITY_INFO, "%.*s", log_line_pos, log_line_buffer.data());
        log_line_pos = 0;
    }

    // Copy data skipping 0 padding
    for (char &c : data) {
        if (c == 0) {
            break;
        }
        log_line_buffer[log_line_pos++] = c;
    }
    while (dispatch_log_event())
        ;
}

CommunicationStatus Dwarf::write_general() {
    CommunicationStatus status = bus.write(unit, GeneralWrite);
    if (status == CommunicationStatus::ERROR) {
        return status;
    }

    DWARF_LOG(LOG_SEVERITY_DEBUG, "Written GeneralWrite");
    return status;
}

CommunicationStatus Dwarf::write_tmc_enable() {
    CommunicationStatus status = bus.write(unit, TmcEnable);
    if (status == CommunicationStatus::ERROR) {
        return status;
    }

    DWARF_LOG(LOG_SEVERITY_DEBUG, "Written TmcEnable");
    return status;
}

uint32_t Dwarf::tmc_read(uint8_t addressByte) {
    // todo: lock!
    TmcReadRequest.value.address = addressByte;
    TmcReadRequest.dirty = true;
    if (bus.write(unit, TmcReadRequest) != CommunicationStatus::ERROR) {
        if (bus.read(unit, TmcReadResponse) != CommunicationStatus::ERROR) {
            DWARF_LOG(LOG_SEVERITY_DEBUG, "TMC on dwarf read (%d:%" PRIu32 ")",
                addressByte, TmcReadResponse.value.value);
            return TmcReadResponse.value.value;
        } else {
            DWARF_LOG(LOG_SEVERITY_ERROR, "TMC read response FAIL");
        }
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "TMC read request FAIL");
    }
    // todo: what to do in case of error?
    return 0;
}

void Dwarf::tmc_write(uint8_t addressByte, uint32_t config) {
    TmcWriteRequest.value.address = addressByte;
    TmcWriteRequest.value.data = config;
    TmcWriteRequest.dirty = true;

    if (bus.write(unit, TmcWriteRequest) != CommunicationStatus::ERROR) {
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Write to TMC dwarf success (%d:%" PRIu32 ")",
            addressByte, config);
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Write to TMC dwarf FAIL");
    }
}

void Dwarf::tmc_set_enable(bool state) {
    uint16_t new_state = state ? 1 : 0;
    if (TmcEnable.value == new_state) {
        return;
    }

    TmcEnable.value = new_state;
    TmcEnable.dirty = true;
    auto result = write_tmc_enable();
    if (result != CommunicationStatus::OK) {
        DWARF_LOG(LOG_SEVERITY_CRITICAL, "Enable pin write error");
    }
}

bool Dwarf::is_tmc_enabled() {
    return TmcEnable.value;
}

float Dwarf::get_hotend_temp() {
    // Sent as int16 in uint16 modbus register
    return static_cast<int16_t>(RegisterGeneralStatus.value.HotendMeasuredTemperature);
}

CommunicationStatus Dwarf::set_hotend_target_temp(float target) {
    GeneralWrite.value.HotendRequestedTemperature = (uint16_t)target;
    GeneralWrite.dirty = true;
    return CommunicationStatus::OK;
}

int Dwarf::get_heater_pwm() {
    return (float)RegisterGeneralStatus.value.HotendPWMState;
}

bool Dwarf::is_picked() const {
    return DiscreteGeneralStatus.value.is_picked;
}

bool Dwarf::is_parked() const {
    return DiscreteGeneralStatus.value.is_parked;
}

bool Dwarf::is_button_up_pressed() const {
    return DiscreteGeneralStatus.value.is_button_up_pressed;
}

bool Dwarf::is_button_down_pressed() const {
    return DiscreteGeneralStatus.value.is_button_down_pressed;
}

CommunicationStatus Dwarf::run_time_sync() {
    RequestTiming timing;
    CommunicationStatus status = bus.read(unit, TimeSync, 1000, &timing);
    if (status == CommunicationStatus::ERROR) {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to read fault status register");
        return status;
    }

    if (status != CommunicationStatus::SKIPPED) {
        time_sync.sync(TimeSync.value.dwarf_time_us, timing);
    }

    return status;
}

[[nodiscard]] bool Dwarf::is_selected() const {
    return selected;
}

CommunicationStatus Dwarf::set_selected(bool selected) {
    // WARNING: this method is called from different thread

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
            if (!set_loadcell(true)) {
                return CommunicationStatus::ERROR;
            }
        }
    } else {
        // Disable accelerometer and loadcell for dwarf being unselected
        if (!set_loadcell(false) || !set_accelerometer(false)) {
            return CommunicationStatus::ERROR;
        }
    }

    return CommunicationStatus::OK;
}

bool Dwarf::set_accelerometer(bool active) {
    // WARNING: this method is called from different thread

    if (active && !this->selected) {
        return false;
    }

    return raw_set_loadcell(!active && this->selected) && raw_set_accelerometer(active);
}

bool Dwarf::set_loadcell(bool active) {
    if (active && !this->selected) {
        return false;
    }

    if (active) {
        return raw_set_accelerometer(false) && raw_set_loadcell(true);
    }

    return raw_set_loadcell(false);
}

bool Dwarf::raw_set_loadcell(bool enable) {
    LoadcellEnableCoil.dirty = true;
    LoadcellEnableCoil.value = enable;
    return bus.write(unit, LoadcellEnableCoil) == CommunicationStatus::OK;
}

bool Dwarf::raw_set_accelerometer(bool enable) {
    AccelerometerEnableCoil.dirty = true;
    AccelerometerEnableCoil.value = enable;
    return bus.write(unit, AccelerometerEnableCoil) == CommunicationStatus::OK;
}

constexpr log_component_t &Dwarf::get_log_component(uint8_t dwarf_nr) {
    switch (dwarf_nr) {
    case 1:
        return __log_component_Dwarf_1;
    case 2:
        return __log_component_Dwarf_2;
    case 3:
        return __log_component_Dwarf_3;
    case 4:
        return __log_component_Dwarf_4;
    case 5:
        return __log_component_Dwarf_5;
    case 6:
        return __log_component_Dwarf_6;
    default:
        bsod("Unknown");
    }
}

IFSensor::value_type Dwarf::get_tool_filament_sensor() {
    // ensure AdcGet::undefined_value is representable within FSensor::value_type
    static_assert(static_cast<IFSensor::value_type>(AdcGet::undefined_value) == AdcGet::undefined_value);

    // widen the type to match the HX717 data type and translate the undefined value for consistency
    IFSensor::value_type value = RegisterGeneralStatus.value.ToolFilamentSensor;
    if (value == AdcGet::undefined_value) {
        value = IFSensor::undefined_value;
    }
    return value;
}

int16_t Dwarf::get_mcu_temperature() {
    // Sent as int16 in uint16 modbus register
    return static_cast<int16_t>(RegisterGeneralStatus.value.MCUTemperature);
}

int16_t Dwarf::get_board_temperature() {
    // Sent as int16 in uint16 modbus register
    return static_cast<int16_t>(RegisterGeneralStatus.value.BoardTemperature);
}

float Dwarf::get_24V() {
    return RegisterGeneralStatus.value.system_24V_mV / 1000.0;
}

float Dwarf::get_heater_current() {
    return RegisterGeneralStatus.value.heater_current_mA / 1000.0;
}

void Dwarf::set_heatbreak_target_temp(int16_t target) {
    GeneralWrite.value.HeatbreakRequestedTemperature = target;
    GeneralWrite.dirty = true;
}

void Dwarf::set_fan(uint8_t fan, uint16_t target) {
    if (GeneralWrite.value.fan_pwm[fan] != target) {
        GeneralWrite.value.fan_pwm[fan] = target;
        GeneralWrite.dirty = true;
    }
}

void Dwarf::set_cheese_led(uint8_t pwr_selected, uint8_t pwr_not_selected) {
    GeneralWrite.value.led_pwm.selected = pwr_selected;
    GeneralWrite.value.led_pwm.not_selected = pwr_not_selected;
    GeneralWrite.dirty = true;
}

void Dwarf::set_cheese_led() {
    set_cheese_led(config_store().tool_leds_enabled.get() ? 0xff : 0x00, 0x00);
}

void Dwarf::set_status_led(dwarf_shared::StatusLed::Mode mode, uint8_t r, uint8_t g, uint8_t b) {
    dwarf_shared::StatusLed status_led(mode, r, g, b);
    GeneralWrite.value.status_led[0] = status_led.get_reg_value(0);
    GeneralWrite.value.status_led[1] = status_led.get_reg_value(1);
    GeneralWrite.dirty = true;
}

void Dwarf::set_pid(float p, float i, float d) {
    // Set the float with one write so it is consistent
    GeneralWrite.value.pid.p = p;
    GeneralWrite.value.pid.i = i;
    GeneralWrite.value.pid.d = d;
    GeneralWrite.dirty = true;
}

void Dwarf::handle_dwarf_fault() {
    // fault is expected when this method is called
    assert(RegisterGeneralStatus.value.FaultStatus != dwarf_shared::errors::FaultStatusMask::NO_FAULT);

    const auto fault_int { ftrstd::to_underlying(RegisterGeneralStatus.value.FaultStatus) };
    DWARF_LOG(LOG_SEVERITY_ERROR, "Fault status: %d", fault_int);

    if (fault_int & ftrstd::to_underlying(dwarf_shared::errors::FaultStatusMask::MARLIN_KILLED)) {
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
        DWARF_LOG(LOG_SEVERITY_ERROR, "Dwarf %d fault %s: %s", dwarf_nr, title_span.data(), message_span.data());

        // Prepare module string (insert dwarf number)
        char module[31] = { 0 };
        snprintf(module, sizeof(module), "Dwarf %d: %s", dwarf_nr, title_span.data());

        // this calls generic fatal error
        // any marlin fault on dwarf will be decoded based on error string and converted to propper ErrCode, or displayed as-is if no error code matches
        fatal_error(message_span.data(), module);
    } else {
        fatal_error(ErrCode::ERR_SYSTEM_DWARF_UNKNOWN_ERR, dwarf_nr);
    }
}

float Dwarf::get_heatbreak_temp() {
    // Sent as int16 in uint16 modbus register
    return static_cast<int16_t>(RegisterGeneralStatus.value.HeatBreakMeasuredTemperature);
}

uint16_t Dwarf::get_heatbreak_fan_pwr() {
    return RegisterGeneralStatus.value.fan[1].pwm;
}

std::array<Dwarf, DWARF_MAX_COUNT> dwarfs { {
    { puppyModbus, 1, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_1) },
    { puppyModbus, 2, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_2) },
    { puppyModbus, 3, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_3) },
    { puppyModbus, 4, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_4) },
    { puppyModbus, 5, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_5) },
    { puppyModbus, 6, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_6) },
} };
} // namespace buddy::puppies
