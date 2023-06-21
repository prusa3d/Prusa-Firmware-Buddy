#include <cassert>
#include <limits>

#include "puppies/Dwarf.hpp"
#include "puppies/fifo_coder.hpp"
#include "puppies/fifo_decoder.hpp"

#include "bsod_gui.hpp"
#include "log.h"
#include "loadcell.h"
#include "timing.h"
#include "puppy/dwarf/loadcell_shared.hpp"
#include "logging/log_dest_bufflog.h"
#include <assert.h>
#include "metric.h"
#include "puppies/PuppyBootstrap.hpp"
#include <i18n.h>
#include "Marlin/src/inc/MarlinConfig.h"
#include "utility_extensions.hpp"
#include "dwarf_errors.hpp"
#include "otp.h"
#include <configuration_store.hpp>
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

using CommunicationStatus = Dwarf::CommunicationStatus;

static metric_t metric_fast_refresh_delay = METRIC("dwarf_fast_refresh_delay", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);

static metric_t metric_dwarf_picked_raw = METRIC("dwarf_picked_raw", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_DISABLE_ALL);
static metric_t metric_dwarf_parked_raw = METRIC("dwarf_parked_raw", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_DISABLE_ALL);

Dwarf::Dwarf(PuppyModbus &bus, const uint8_t dwarf_nr, uint8_t modbus_address)
    : ModbusDevice(bus, modbus_address)
    , dwarf_nr(dwarf_nr)
    , log_component(get_log_component(dwarf_nr))
    , selected(false)
    , time_sync(this->dwarf_nr)
    , callbacks(Decoder::Callbacks_t {
          .log_handler = std::bind(&Dwarf::handle_log_fragment, this, std::placeholders::_1),
          .loadcell_handler = [this](LoadcellRecord data) {
              // throw away samples if time is not synced
              if (!this->time_sync.is_time_sync_valid() || !this->selected)
                  return;

              loadcell.ProcessSample(data.loadcell_raw_value, this->time_sync.buddy_time_us(data.timestamp)); },
          .accelerometer_handler = [this](AccelerometerData data) {
              // throw away samples if time is not synced
              if (!this->time_sync.is_time_sync_valid() || !this->selected)
                  return;
              PrusaAccelerometer::put_sample(data.sample);
              report_accelerometer(1); },
          .accelerometer_fast_handler = [this](AccelerometerFastData data) {
              // throw away samples if not selected
              if (!this->is_selected())
                  return;
              for (AccelerometerXyzSample sample : data) {
                  PrusaAccelerometer::put_sample(sample);
              }
              report_accelerometer(data.size()); } }) {

    RegisterGeneralStatus.value.FaultStatus = dwarf_shared::errors::FaultStatusMask::NO_FAULT;
    RegisterGeneralStatus.value.HotendMeasuredTemperature = 6;
    RegisterGeneralStatus.value.HeatBreakMeasuredTemperature = 6;

    RegisterGeneralStatus.value.HotendPWMState = 0;

    DiscreteGeneralStatus.value.is_picked = false;
    DiscreteGeneralStatus.value.is_parked = false;

    GeneralWrite.value.HotendRequestedTemperature = 0;
    GeneralWrite.value.HeatbreakRequestedTemperature = DEFAULT_HEATBREAK_TEMPERATURE;

    set_led(); // Set LED by eeprom config
}

CommunicationStatus Dwarf::refresh(uint32_t cycle_ticks_ms, bool &worked) {
    CommunicationStatus status = CommunicationStatus::OK;

    // read something every 200 ms
    if (last_update_ms + DWARF_READ_PERIOD > cycle_ticks_ms) {
        worked = false;
        return CommunicationStatus::OK;
    }
    last_update_ms = cycle_ticks_ms;
    worked = true;

    switch (refresh_nr) {
    case 0: {
        // read general status registers
        if (modbusIsOk(bus.read(unit, RegisterGeneralStatus))) {
            if (RegisterGeneralStatus.value.FaultStatus != dwarf_shared::errors::FaultStatusMask::NO_FAULT) {
                handle_dwarf_fault();
            }

            metric_record_custom(&metric_dwarf_parked_raw, ",n=%u v=%ii", dwarf_nr, RegisterGeneralStatus.value.IsParkedRaw);
            metric_record_custom(&metric_dwarf_picked_raw, ",n=%u v=%ii", dwarf_nr, RegisterGeneralStatus.value.IsPickedRaw);
        } else {
            DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to read fault status register");
            status = CommunicationStatus::ERROR;
        }
        break;
    }
    case 1: {
        status = read_discrete_general_status();
        break;
    }
    case 2: {
        if (GeneralWriteNeedWrite) {
            status = write_general();
        }
        break;
    }
    case 3: {
        if (TmcEnable.pending) {
            status = write_tmc_enable();
        }
        break;
    }
    case 4: {
        run_time_sync();
        break;
    }
    default: {
        refresh_nr = -1;
    }
    }

    refresh_nr++;

    return status;
}

CommunicationStatus Dwarf::read_discrete_general_status() {
    // read general status discrete inputs
    if (modbusIsOk(bus.read(unit, DiscreteGeneralStatus))) {
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Is parked: %d", DiscreteGeneralStatus.value.is_parked);
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Is picked: %d", DiscreteGeneralStatus.value.is_picked);
        return CommunicationStatus::OK;
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to read fault status register");
        return CommunicationStatus::ERROR;
    }
}

CommunicationStatus Dwarf::ping() {
    return modbusIsOk(bus.read(unit, GeneralStatic)) ? CommunicationStatus::OK : CommunicationStatus::ERROR;
}

CommunicationStatus Dwarf::initial_scan() {
    time_sync.init();
    run_time_sync();

    bool communication_error = false;
    // Update static values
    if (modbusIsOk(bus.read(unit, GeneralStatic))) {
        DWARF_LOG(LOG_SEVERITY_INFO, "HwBomId: %d", GeneralStatic.value.HwBomId);
        DWARF_LOG(LOG_SEVERITY_INFO, "HwOtpTimestsamp: %d", GeneralStatic.value.HwOtpTimestsamp);

        serial_nr_t sn = {}; // Last byte has to be '\0'
        static constexpr uint16_t raw_datamatrix_regsize = ftrstd::to_underlying(SystemInputRegister::hw_raw_datamatrix_last)
            - ftrstd::to_underlying(SystemInputRegister::hw_raw_datamatrix_first) + 1;
        // Check size of text -1 as the terminating \0 is not sent
        static_assert((raw_datamatrix_regsize * sizeof(uint16_t)) == (sizeof(sn.txt) - 1), "Size of raw datamatrix doesn't fit modbus registers");

        for (uint16_t i = 0; i < raw_datamatrix_regsize; ++i) {
            sn.txt[i * 2] = GeneralStatic.value.HwDatamatrix[i] & 0xff;
            sn.txt[i * 2 + 1] = GeneralStatic.value.HwDatamatrix[i] >> 8;
        }
        DWARF_LOG(LOG_SEVERITY_INFO, "HwDatamatrix: %s", sn.txt);
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to read static general register pack");
        communication_error = true;
    }

    // read discrete general stats - contins data about picked/parked, and that is needed immediately upon init to pick correct tool
    if (read_discrete_general_status() != CommunicationStatus::OK) {
        communication_error = true;
    }

    GeneralWriteNeedWrite = true;
    TmcEnable.pending = true;
    selected = false;

    return communication_error ? CommunicationStatus::ERROR : CommunicationStatus::OK;
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

CommunicationStatus Dwarf::fifo_refresh(uint32_t cycle_ticks_ms, bool &worked) {
    // pull fifo every 200 ms
    if (last_pull_ms + DWARF_FIFO_PULL_PERIOD > cycle_ticks_ms) {
        worked = false;
        return CommunicationStatus::OK;
    }
    worked = true;

    bool more;
    auto ret = pull_fifo(more);
    if (!more && ret == CommunicationStatus::OK) {
        last_pull_ms = cycle_ticks_ms; // Wait before next pull only if all is read
    }
    return ret;
}

CommunicationStatus Dwarf::pull_fifo(bool &more) {
    // Read coded FIFO
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo;
    size_t read = 0;
    if (modbusIsOk(bus.ReadFIFO(unit, ENCODED_FIFO_ADDRESS, fifo, read))) {
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

        more = decoder.more();
        return CommunicationStatus::OK;
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to read coded FIFO");
        return CommunicationStatus::ERROR;
    }
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
    if (modbusIsOk(bus.write(unit, GeneralWrite))) {
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Written GeneralWrite");
        GeneralWriteNeedWrite = false;
        return CommunicationStatus::OK;
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to write GeneralWrite");
        return CommunicationStatus::ERROR;
    }
}

CommunicationStatus Dwarf::write_tmc_enable() {
    if (modbusIsOk(bus.write(unit, TmcEnable))) {
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Written TmcEnable");
        TmcEnable.pending = false;
        return CommunicationStatus::OK;
    } else {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to write TmcEnable");
        return CommunicationStatus::ERROR;
    }
}

uint32_t Dwarf::tmc_read(uint8_t addressByte) {
    // todo: lock!
    TmcReadRequest.value.address = addressByte;
    if (modbusIsOk(bus.write(unit, TmcReadRequest))) {
        if (modbusIsOk(bus.read(unit, TmcReadResponse))) {
            DWARF_LOG(LOG_SEVERITY_DEBUG, "TMC on dwarf read (%i:%i)", addressByte, TmcReadResponse.value.value);
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

    if (modbusIsOk(bus.write(unit, TmcWriteRequest))) {
        DWARF_LOG(LOG_SEVERITY_DEBUG, "Write to TMC dwarf success (%i:%i)", addressByte, config);
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
    TmcEnable.pending = true;
    auto result = write_tmc_enable();
    if (result != CommunicationStatus::OK) {
        DWARF_LOG(LOG_SEVERITY_CRITICAL, "Enable pin write error");
    }
}

bool Dwarf::is_tmc_enabled() {
    return TmcEnable.value;
}

float Dwarf::get_hotend_temp() {
    return (float)RegisterGeneralStatus.value.HotendMeasuredTemperature;
}

CommunicationStatus Dwarf::set_hotend_target_temp(float target) {
    GeneralWrite.value.HotendRequestedTemperature = (uint16_t)target;
    GeneralWriteNeedWrite = true;
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

CommunicationStatus Dwarf::run_time_sync() {
    RequestTiming timing;
    if (!modbusIsOk(bus.read(unit, TimeSync, &timing))) {
        DWARF_LOG(LOG_SEVERITY_ERROR, "Failed to read fault status register");
        return CommunicationStatus::ERROR;
    }

    time_sync.sync(TimeSync.value.dwarf_time_us, timing);

    return CommunicationStatus::OK;
}

[[nodiscard]] bool Dwarf::is_selected() const {
    return selected;
}

CommunicationStatus Dwarf::set_selected(bool selected) {
    // WARNING: this method is called from different thread

    IsSelectedCoil.pending = true;
    IsSelectedCoil.value = selected;
    if (!modbusIsOk(bus.write(unit, IsSelectedCoil))) {
        return CommunicationStatus::ERROR;
    }
    LoadcellEnableCoil.pending = true;
    LoadcellEnableCoil.value = selected;
    if (!modbusIsOk(bus.write(unit, LoadcellEnableCoil))) {
        return CommunicationStatus::ERROR;
    }
    AccelerometerEnableCoil.pending = true;
    AccelerometerEnableCoil.value = false;
    if (!modbusIsOk(bus.write(unit, AccelerometerEnableCoil))) {
        return CommunicationStatus::ERROR;
    }
    AccelerometerHighCoil.pending = true;
    AccelerometerHighCoil.value = false;
    if (!modbusIsOk(bus.write(unit, AccelerometerHighCoil))) {
        return CommunicationStatus::ERROR;
    }
    this->selected = selected;
    return CommunicationStatus::OK;
}

/**
 * @brief Set accelerometer
 *
 * @param active
 *  - true Activate accelerometer in high sample rate and deactivate load cell
 *  - false Deactivate accelerometer in high sample rate and activate load cell
 * @retval true success
 * @retval false failed - either communication error or Dwarf not selected
 */
bool Dwarf::set_accelerometer(bool active) {
    // WARNING: this method is called from different thread

    if (!this->selected) {
        return false;
    }

    if (active) {
        LoadcellEnableCoil.pending = true;
        LoadcellEnableCoil.value = false;
        if (!modbusIsOk(bus.write(unit, LoadcellEnableCoil))) {
            return false;
        }
        AccelerometerEnableCoil.pending = true;
        AccelerometerEnableCoil.value = true;
        if (!modbusIsOk(bus.write(unit, AccelerometerEnableCoil))) {
            return false;
        }
        AccelerometerHighCoil.pending = true;
        AccelerometerHighCoil.value = true;
        if (!modbusIsOk(bus.write(unit, AccelerometerHighCoil))) {
            return false;
        }
    } else {
        LoadcellEnableCoil.pending = true;
        LoadcellEnableCoil.value = true;
        if (!modbusIsOk(bus.write(unit, LoadcellEnableCoil))) {
            return false;
        }
        AccelerometerEnableCoil.pending = true;
        AccelerometerEnableCoil.value = false;
        if (!modbusIsOk(bus.write(unit, AccelerometerEnableCoil))) {
            return false;
        }
        AccelerometerHighCoil.pending = true;
        AccelerometerHighCoil.value = false;
        if (!modbusIsOk(bus.write(unit, AccelerometerHighCoil))) {
            return false;
        }
    }
    return true;
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

int32_t Dwarf::get_tool_filament_sensor() {
    return RegisterGeneralStatus.value.ToolFilamentSensor;
}

uint16_t Dwarf::get_mcu_temperature() {
    return RegisterGeneralStatus.value.MCU_temperature;
}

void Dwarf::set_heatbreak_target_temp(int16_t target) {
    GeneralWrite.value.HeatbreakRequestedTemperature = target;
    GeneralWriteNeedWrite = true;
}

void Dwarf::set_fan(uint8_t fan, uint16_t target) {
    if (GeneralWrite.value.fan_pwm[fan] != target) {
        GeneralWrite.value.fan_pwm[fan] = target;
        GeneralWriteNeedWrite = true;
    }
}

void Dwarf::set_led(uint8_t pwr_selected, uint8_t pwr_not_selected) {
    GeneralWrite.value.led_pwm.selected = pwr_selected;
    GeneralWrite.value.led_pwm.not_selected = pwr_not_selected;
    GeneralWriteNeedWrite = true;
}

void Dwarf::set_led() {
    set_led(config_store().tool_leds_enabled.get() ? 0xff : 0x00, 0x00);
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

        if (!modbusIsOk(bus.read(unit, MarlinErrorString))) {
            // read failed, make it empty string
            title_span[0] = '\0';
            message_span[0] = '\0';
        }
        // make sure strings are zero-terminated
        title_span.back() = 0;
        message_span.back() = 0;
        DWARF_LOG(LOG_SEVERITY_ERROR, "Dwarf %d fault %s: %s", dwarf_nr, title_span, message_span);

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
    return RegisterGeneralStatus.value.HeatBreakMeasuredTemperature;
}

uint16_t Dwarf::get_heatbreak_fan_pwr() {
    return RegisterGeneralStatus.value.fan[1].pwm;
}

void Dwarf::report_accelerometer(int samples_received) {
    static uint32_t last_report = 0;
    static uint32_t sample_count = 0;
    sample_count += samples_received;
    if (ticks_ms() - last_report > 1000) {
        DWARF_LOG(LOG_SEVERITY_INFO, "acc sample rate: %u", sample_count);
        sample_count = 0;
        last_report = ticks_ms();
    }
}

std::array<Dwarf, DWARF_MAX_COUNT> dwarfs { {
    { puppyModbus, 1, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_1) },
    { puppyModbus, 2, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_2) },
    { puppyModbus, 3, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_3) },
    { puppyModbus, 4, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_4) },
    { puppyModbus, 5, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_5) },
    { puppyModbus, 6, PuppyBootstrap::get_modbus_address_for_dock(Dock::DWARF_6) },
} };
}
