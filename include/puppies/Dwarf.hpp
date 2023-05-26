#pragma once

#include <limits>
#include <array>
#include <atomic>

#include "puppies/PuppyModbus.hpp"
#include "puppies/fifo_decoder.hpp"
#include "puppies/puppy_constants.hpp"
#include "puppies/time_sync.hpp"
#include <include/dwarf_registers.hpp>
#include <utils/utility_extensions.hpp>
#include <include/dwarf_errors.hpp>

using namespace common::puppies::fifo;

namespace buddy::puppies {

class Dwarf : public ModbusDevice {
public:
    using SystemDiscreteInput = dwarf_shared::registers::SystemDiscreteInput;
    using SystemCoil = dwarf_shared::registers::SystemCoil;
    using SystemInputRegister = dwarf_shared::registers::SystemInputRegister;
    using SystemHoldingRegister = dwarf_shared::registers::SystemHoldingRegister;
    using SystemFIFO = dwarf_shared::registers::SystemFIFO;

    static constexpr uint16_t GENERAL_DISCRETE_INPUTS_ADDR { ftrstd::to_underlying(SystemDiscreteInput::is_picked) };

    static constexpr uint16_t TMC_ENABLE_ADDR { ftrstd::to_underlying(SystemCoil::tmc_enable) };
    static constexpr uint16_t IS_SELECTED { ftrstd::to_underlying(SystemCoil::is_selected) };
    static constexpr uint16_t LOADCELL_ENABLE { ftrstd::to_underlying(SystemCoil::loadcell_enable) };
    static constexpr uint16_t ACCELEROMETER_ENABLE { ftrstd::to_underlying(SystemCoil::accelerometer_enable) };
    static constexpr uint16_t ACCELEROMETER_HIGH { ftrstd::to_underlying(SystemCoil::accelerometer_high) };

    static constexpr uint16_t HW_BOM_ID_ADDR { ftrstd::to_underlying(SystemInputRegister::hw_bom_id) };
    static constexpr uint16_t TMC_READ_RESPONSE_ADDRESS { ftrstd::to_underlying(SystemInputRegister::tmc_read_response_1) };
    static constexpr uint16_t FAULT_STATUS_ADDR { ftrstd::to_underlying(SystemInputRegister::fault_status) };
    static constexpr uint16_t TIME_SYNC_ADDR { ftrstd::to_underlying(SystemInputRegister::time_sync_lo) };
    static constexpr uint16_t MARLIN_ERROR_COMPONENT_START { ftrstd::to_underlying(SystemInputRegister::marlin_error_component_start) };

    static constexpr uint16_t GENERAL_WRITE_REQUEST { ftrstd::to_underlying(SystemHoldingRegister::nozzle_target_temperature) };
    static constexpr uint16_t TMC_READ_REQUEST_ADDRESS { ftrstd::to_underlying(SystemHoldingRegister::tmc_read_request) };
    static constexpr uint16_t TMC_WRITE_REQUEST_ADDRESS { ftrstd::to_underlying(SystemHoldingRegister::tmc_write_request_address) };
    static constexpr uint16_t ENCODED_FIFO_ADDRESS { ftrstd::to_underlying(SystemFIFO::encoded_stream) };

    static constexpr uint32_t DWARF_READ_PERIOD = 200;
    static constexpr uint_fast8_t NUM_FANS = 2;

    /// when this is set as PWM, fan is switched to automatic mode
    static constexpr uint16_t FAN_MODE_AUTO_PWM = std::numeric_limits<uint16_t>::max();

public:
    enum class CommunicationStatus {
        OK,
        ERROR,
    };

    Dwarf(PuppyModbus &bus, const uint8_t dwarf_nr, uint8_t modbus_address);
    Dwarf(const Dwarf &) = delete;

    CommunicationStatus ping();
    CommunicationStatus initial_scan();
    CommunicationStatus refresh();
    CommunicationStatus fast_refresh();

    [[nodiscard]] bool is_selected() const;
    CommunicationStatus set_selected(bool selected);
    bool set_accelerometer(bool active);

    uint32_t tmc_read(uint8_t addressByte);
    void tmc_write(uint8_t addressByte, uint32_t config);

    void tmc_set_enable(bool state);
    bool is_tmc_enabled();

    CommunicationStatus set_hotend_target_temp(float target);
    float get_hotend_temp();
    int get_heater_pwm();

    bool is_picked();
    bool is_parked();

    inline void refresh_park_pick_status() {
        (void)read_discrete_general_status();
    }

    int32_t get_tool_filament_sensor();
    uint16_t get_mcu_temperature();

    void set_heatbreak_target_temp(int16_t target);
    void set_fan(uint8_t fan, uint16_t target);
    float get_heatbreak_temp();
    uint16_t get_heatbreak_fan_pwr();

    MODBUS_REGISTER GeneralStatic_t {
        uint16_t HwBomId;
        uint32_t HwOtpTimestsamp;
        uint16_t HwDatamatrix[12];
    };
    ModbusInputRegisterBlock<HW_BOM_ID_ADDR, GeneralStatic_t> GeneralStatic;

    MODBUS_DISCRETE DiscreteGeneralStatus_t {
        bool is_picked;
        bool is_parked;
    };
    ModbusDiscreteInputBlock<GENERAL_DISCRETE_INPUTS_ADDR, DiscreteGeneralStatus_t> DiscreteGeneralStatus;

    MODBUS_REGISTER FanReadRegisters {
        uint16_t rpm;
        uint16_t pwm;
        uint16_t state;
        uint16_t is_rpm_ok;
    };

    MODBUS_REGISTER RegisterGeneralStatus_t {
        dwarf_shared::errors::FaultStatusMask FaultStatus;
        uint16_t HotendMeasuredTemperature;
        uint16_t HotendPWMState;
        uint16_t ToolFilamentSensor;
        uint16_t MCU_temperature;
        uint16_t HeatBreakMeasuredTemperature;
        uint16_t IsPickedRaw;
        uint16_t IsParkedRaw;
        FanReadRegisters fan[NUM_FANS];
    };
    ModbusInputRegisterBlock<FAULT_STATUS_ADDR, RegisterGeneralStatus_t> RegisterGeneralStatus;

    MODBUS_REGISTER TimeSync_t {
        uint32_t dwarf_time_us;
    };
    ModbusInputRegisterBlock<TIME_SYNC_ADDR, TimeSync_t> TimeSync;

    MODBUS_REGISTER GeneralWrite_t {
        uint16_t HotendRequestedTemperature;
        uint16_t HeatbreakRequestedTemperature;

        static constexpr uint16_t FAN_AUTO_PWM = std::numeric_limits<uint16_t>::max();
        uint16_t fan_pwm[NUM_FANS]; // target PWM or when value is FAN_AUTO_RPM, use automatic control
    };
    ModbusHoldingRegisterBlock<GENERAL_WRITE_REQUEST, GeneralWrite_t> GeneralWrite;
    bool GeneralWriteNeedWrite = true; // set this to true, when GeneralWrite is requested to be written

    MODBUS_REGISTER TmcWriteRequest_t {
        uint16_t address;
        uint32_t data;
    };
    ModbusHoldingRegisterBlock<TMC_WRITE_REQUEST_ADDRESS, TmcWriteRequest_t> TmcWriteRequest;

    MODBUS_REGISTER TmcReadRequest_t {
        uint16_t address;
    };
    ModbusHoldingRegisterBlock<TMC_READ_REQUEST_ADDRESS, TmcReadRequest_t> TmcReadRequest;

    MODBUS_REGISTER TmcReadResponse_t {
        uint32_t value;
    };
    ModbusInputRegisterBlock<TMC_READ_RESPONSE_ADDRESS, TmcReadResponse_t> TmcReadResponse;

    ModbusCoil<TMC_ENABLE_ADDR> TmcEnable;
    ModbusCoil<IS_SELECTED> IsSelectedCoil;
    ModbusCoil<LOADCELL_ENABLE> LoadcellEnableCoil;
    ModbusCoil<ACCELEROMETER_ENABLE> AccelerometerEnableCoil;
    ModbusCoil<ACCELEROMETER_HIGH> AccelerometerHighCoil;

    MODBUS_REGISTER MarlinErrorString_t {
        uint16_t title[10];   // 20 chars, title of error
        uint16_t message[32]; // 64 chars, message of error
    };
    ModbusInputRegisterBlock<MARLIN_ERROR_COMPONENT_START, MarlinErrorString_t> MarlinErrorString;

    inline uint8_t get_dwarf_nr() const {
        return dwarf_nr;
    }

private:
    /// @brief Dwarf number (1-5)
    uint8_t dwarf_nr;

    /// @brief Log component asociated with this dwarf
    log_component_t &log_component;

    /// @brief True means this tool is picked and active
    std::atomic<bool> selected;

    // Log transfer buffer and position
    std::array<char, 256> log_line_buffer;
    size_t log_line_pos = 0;
    buddy::puppies::TimeSync time_sync;

    const Decoder::Callbacks_t callbacks;
    CommunicationStatus write_general();
    CommunicationStatus write_tmc_enable();
    CommunicationStatus pull_log_fifo();
    CommunicationStatus pull_loadcell_fifo();
    bool dispatch_log_event();
    void handle_log_fragment(LogData data);
    CommunicationStatus run_time_sync();
    constexpr log_component_t &get_log_component(uint8_t dwarf_nr);
    CommunicationStatus read_discrete_general_status();
    void handle_dwarf_fault();
    void report_accelerometer(int samples_received);

    // Register refresh control
    uint32_t last_update_ms = 0;
    uint32_t refresh_nr = 0;
};

extern std::array<Dwarf, DWARF_MAX_COUNT> dwarfs;

}
