#pragma once

#include <limits>
#include <array>
#include <atomic>
#include <memory>

#include "puppies/PuppyModbus.hpp"
#include <fifo_coder/fifo_decoder.hpp>
#include <logging/log.hpp>
#include "puppies/puppy_constants.hpp"
#include "puppies/time_sync.hpp"
#include <include/dwarf_registers.hpp>
#include <utils/utility_extensions.hpp>
#include <puppies/dwarf_status_led.hpp>
#include <include/dwarf_errors.hpp>
#include <feature/filament_sensor/filament_sensor.hpp>
#include <timing.h>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

namespace freertos {
class Mutex;
}

using namespace fifo_coder;

namespace buddy::puppies {

class Dwarf final : public ModbusDevice, public Decoder::Callbacks {
public:
    using SystemDiscreteInput = dwarf_shared::registers::SystemDiscreteInput;
    using SystemCoil = dwarf_shared::registers::SystemCoil;
    using SystemInputRegister = dwarf_shared::registers::SystemInputRegister;
    using SystemHoldingRegister = dwarf_shared::registers::SystemHoldingRegister;
    using SystemFIFO = dwarf_shared::registers::SystemFIFO;

    static constexpr uint16_t GENERAL_DISCRETE_INPUTS_ADDR { std::to_underlying(SystemDiscreteInput::is_picked) };

    static constexpr uint16_t TMC_ENABLE_ADDR { std::to_underlying(SystemCoil::tmc_enable) };
    static constexpr uint16_t IS_SELECTED { std::to_underlying(SystemCoil::is_selected) };
    static constexpr uint16_t LOADCELL_ENABLE { std::to_underlying(SystemCoil::loadcell_enable) };
    static constexpr uint16_t ACCELEROMETER_ENABLE { std::to_underlying(SystemCoil::accelerometer_enable) };

    static constexpr uint16_t HW_BOM_ID_ADDR { std::to_underlying(SystemInputRegister::hw_bom_id) };
    static constexpr uint16_t TMC_READ_RESPONSE_ADDRESS { std::to_underlying(SystemInputRegister::tmc_read_response_1) };
    static constexpr uint16_t FAULT_STATUS_ADDR { std::to_underlying(SystemInputRegister::fault_status) };
    static constexpr uint16_t TIME_SYNC_ADDR { std::to_underlying(SystemInputRegister::time_sync_lo) };
    static constexpr uint16_t MARLIN_ERROR_COMPONENT_START { std::to_underlying(SystemInputRegister::marlin_error_component_start) };

    static constexpr uint16_t GENERAL_WRITE_REQUEST { std::to_underlying(SystemHoldingRegister::nozzle_target_temperature) };
    static constexpr uint16_t TMC_READ_REQUEST_ADDRESS { std::to_underlying(SystemHoldingRegister::tmc_read_request) };
    static constexpr uint16_t TMC_WRITE_REQUEST_ADDRESS { std::to_underlying(SystemHoldingRegister::tmc_write_request_address) };
    static constexpr uint16_t ENCODED_FIFO_ADDRESS { std::to_underlying(SystemFIFO::encoded_stream) };

    static constexpr uint32_t DWARF_READ_PERIOD = 200; ///< Read registers this often [ms]
    static constexpr uint32_t DWARF_FIFO_PULL_PERIOD = 200; ///< Pull fifo of unselected dwarf this often [ms]
    static constexpr uint_fast8_t NUM_FANS = 2;
    static constexpr uint8_t FIFO_RETRIES = 3;

    /// when this is set as PWM, fan is switched to automatic mode
    static constexpr uint16_t FAN_MODE_AUTO_PWM = std::numeric_limits<uint16_t>::max();

public:
    Dwarf(const uint8_t dwarf_number, uint8_t modbus_address);
    Dwarf(const Dwarf &) = delete;

    CommunicationStatus ping(PuppyModbus &);
    CommunicationStatus initial_scan(PuppyModbus &);

    /**
     * @brief Refreshes all registers from dwarf.
     * @return CommunicationStatus::OK on successful refresh and
     *   CommunicationStatus::SKIPPED on successful skip.
     */
    CommunicationStatus refresh(PuppyModbus &);

    /**
     * @brief Pulls data from dwarf fifo, but is timed for non-selected dwarf.
     * @param cycle_ticks_ms ticks_ms() valid through current poll cycle [ms]
     * @param[out] worked true if fifo was pulled, false if not yet
     * @return CommunicationStatus::OK on success and
     *   CommunicationStatus::SKIPPED on successful skip.
     */
    CommunicationStatus fifo_refresh(PuppyModbus &, uint32_t cycle_ticks_ms);

    /**
     * @brief Pulls data from dwarf fifo.
     * Originally fast_refresh().
     * @param[out] more true if there is more data to pull, false if fifo is empty
     * @return CommunicationStatus::OK on success
     */
    CommunicationStatus pull_fifo(PuppyModbus &, bool &more);

    [[nodiscard]] bool is_selected() const;

    /**
     * @brief Control dwarf selected state
     *
     * Consider dwarf selected/unselected for buddy
     * Configure dwarf as selected/unselected registers
     * Enable loadcell (if not already using accelerometer) on select
     * Disable loadcell or accelerometer on unselect
     *
     * @param selected Selection state bool
     * @return CommunicationStatus::OK on success
     */
    CommunicationStatus set_selected(PuppyModbus &, bool selected);

    /**
     * @brief Set loadcell
     *
     * Can be enabled only on selected dwarf.
     * Automatically disable accelerometer on loadcell activation.
     *
     * @param active Loadcell state bool
     * @return True when successful, false otherwise (either communication error or Dwarf not selected)
     */
    bool set_loadcell(PuppyModbus &, bool active);

    /**
     * @brief Set accelerometer
     *
     * Can be enabled only on selected dwarf.
     * Automatically disable loadcell on accelerometer activation.
     * Automatically enable loadcell on accelerometer deactivation on a selected dwarf.
     * Accelerometer is always enabled using high sample rate.
     *
     * @param active Accelerometer state bool
     * @return True when successful, false otherwise (either communication error or Dwarf not selected)
     */
    bool set_accelerometer(PuppyModbus &, bool active);

    uint32_t tmc_read(PuppyModbus &, uint8_t addressByte);
    void tmc_write(PuppyModbus &, uint8_t addressByte, uint32_t config);

    void tmc_set_enable(PuppyModbus &, bool state);
    bool is_tmc_enabled();

    CommunicationStatus set_hotend_target_temp(float target);
    float get_hotend_temp();
    int get_heater_pwm();

    /**
     * @brief Refresh dwarf discrete general status.
     * This is intended for calling from other threads.
     * It marks the last read timestamp as old, so that the next refresh will read it.
     * It can take several ms before the value gets updated.
     * @return true on success
     */
    [[nodiscard]] inline bool refresh_discrete_general_status() {
        // Preset very old value to timestamp so it passes any age check in refresh()
        discrete_general_status_last_read_ms = last_ticks_ms() - (std::numeric_limits<uint32_t>::max() / 2);
        return true;
    }

    bool is_picked() const;
    bool is_parked() const;
    inline bool refresh_park_pick_status() { return refresh_discrete_general_status(); }

    /// @return true if upper button is pressed, needs refreshing by refresh_buttons()
    bool is_button_up_pressed() const;

    /// @return true if lower button is pressed, needs refreshing by refresh_buttons()
    bool is_button_down_pressed() const;

    /**
     * @brief Refresh dwarf button state.
     * @return true on success
     *
     * @note Dwarf buttons need to be refreshed more often.
     * The button states are obtained only once ~250 ms which is not enough for user interaction.
     * To get usable button state, call refresh_buttons() periodically.
     * For example in GUI_event_t::LOOP at ~50 ms.
     * The states are processed first and then marked to be refreshed which happens until next loop call.
     * @code
void ToolsMappingBody::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    if (event == GUI_event_t::LOOP) {
        // Process dwarf buttons
        static bool was_up = false, was_down = false;
        bool is_up = false, is_down = false;
        for (auto tool : PhysicalToolIndex::all()) {
            if (prusa_toolchanger.getTool(tool).is_button_up_pressed()) { is_up = true; }
            if (prusa_toolchanger.getTool(tool).is_button_down_pressed()) { is_down = true; }
        }
        if (is_up && !was_up) {
            index--; // Some activity on up button pressed
        }
        if (is_down && !was_down) {
            index++; // Some activity on down button pressed
        }
        was_up = is_up;
        was_down = is_down;

        // Refresh until next loop
        for (auto tool : PhysicalToolIndex::all()) {
            prusa_toolchanger.getTool(tool).refresh_buttons();
        }
        break;
    }
}
     * @endcode
     */
    inline bool refresh_buttons() { return refresh_discrete_general_status(); }

    [[nodiscard]] IFSensor::value_type get_tool_filament_sensor();

    /// @returns a value that increments every time the puppy is reset
    /// It is recommended to check for the reset counter against the previous one
    /// AFTER reading out all the needed data from the puppy.
    [[nodiscard]] uint32_t get_reset_counter() const {
        return reset_counter.load();
    }

    [[nodiscard]] int16_t get_mcu_temperature(); ///< Get MCU temperature [°C]
    [[nodiscard]] int16_t get_board_temperature(); ///< Get board temperature [°C]
    [[nodiscard]] float get_24V(); ///< Get 24V power supply voltage [V]
    [[nodiscard]] float get_heater_current(); ///< Get heater electric current [A]

    void set_heatbreak_target_temp(int16_t target);
    void set_fan(uint8_t fan, uint16_t target);

    /**
     * @brief Set fan to auto mode.
     * @param fan fan number
     */
    void set_fan_auto(uint8_t fan);

    enum class FanMode : uint16_t {
        XL_LEGACY = 0, ///< XL/legacy mode
        XLS_NATIVE = 1, ///< XLS native mode
        XLS_COMPAT = 2, ///< XLS compat mode
    };

    void set_fan_mode(FanMode mode);

    /**
     * @brief Set cheese LED.
     * @param pwr_selected PWM when selected [0 - 255]
     * @param pwr_not_selected PWM when not selected [0 - 255]
     */
    void set_cheese_led(uint8_t pwr_selected, uint8_t pwr_not_selected);

    /**
     * @brief Set cheese LED by eeprom config.
     */
    void set_cheese_led();

    /**
     * @brief Set dwarf status LED to pulse.
     * @param mode select solid, flashing or pulsing
     * @param r red [0 - 255]
     * @param g green [0 - 255]
     * @param b blue [0 - 255]
     */
    void set_status_led(dwarf_shared::StatusLed::Mode mode, uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Set dwarf status LED to default, to follow internal state.
     */
    inline void set_status_led() {
        set_status_led(dwarf_shared::StatusLed::Mode::dwarf_status, 0, 0, 0);
    }

    /**
     * @brief Set dwarf status LED to solid color.
     * @param r red [0 - 255]
     * @param g green [0 - 255]
     * @param b blue [0 - 255]
     */
    inline void set_status_led_solid(uint8_t r, uint8_t g, uint8_t b) {
        set_status_led(dwarf_shared::StatusLed::Mode::solid_color, r, g, b);
    }

    /**
     * @brief Set turn off dwarf status LED.
     */
    inline void set_status_led_off() { set_status_led_solid(0, 0, 0); }

    /**
     * @brief Set dwarf status LED to blink.
     * @param r red [0 - 255]
     * @param g green [0 - 255]
     * @param b blue [0 - 255]
     */
    inline void set_status_led_blink(uint8_t r, uint8_t g, uint8_t b) {
        set_status_led(dwarf_shared::StatusLed::Mode::blink, r, g, b);
    }

    /**
     * @brief Set dwarf status LED to pulse.
     * @param r red [0 - 255]
     * @param g green [0 - 255]
     * @param b blue [0 - 255]
     */
    inline void set_status_led_pulse(uint8_t r, uint8_t g, uint8_t b) {
        set_status_led(dwarf_shared::StatusLed::Mode::pulse, r, g, b);
    }

    /**
     * @brief Set dwarf status LED to pulse to white.
     * @param r red [0 - 255]
     * @param g green [0 - 255]
     * @param b blue [0 - 255]
     */
    inline void set_status_led_pulse_white(uint8_t r, uint8_t g, uint8_t b) {
        set_status_led(dwarf_shared::StatusLed::Mode::pulse_w, r, g, b);
    }

    /**
     * @brief Set dwarf heater PID values.
     * @param p format as Temperature::temp_hotend[0].pid.Kp
     * @param i format as Temperature::temp_hotend[0].pid.Ki
     * @param d format as Temperature::temp_hotend[0].pid.Kd
     */
    void set_pid(float p, float i, float d);

    float get_heatbreak_temp();
    uint16_t get_heatbreak_fan_pwr();

    inline uint8_t dwarf_index() const {
        return dwarf_number;
    }
    PhysicalToolIndex tool_index() const {
        return PhysicalToolIndex::from_raw(dwarf_number);
    }

    uint16_t get_fan_pwm(uint8_t fan_nr) const;
    uint16_t get_fan_rpm(uint8_t fan_nr) const;
    bool get_fan_rpm_ok(uint8_t fan_nr) const;
    uint16_t get_fan_state(uint8_t fan_nr) const;

private:
    MODBUS_REGISTER GeneralStatic_t {
        uint16_t HwBomId {};
        uint32_t HwOtpTimestsamp {};
        uint16_t HwDatamatrix[12] {};
    };
    ModbusInputRegisterBlock<HW_BOM_ID_ADDR, GeneralStatic_t> GeneralStatic {};

    MODBUS_DISCRETE DiscreteGeneralStatus_t {
        bool is_picked {};
        bool is_parked {};
        bool is_button_up_pressed {};
        bool is_button_down_pressed {};
    };

    MODBUS_REGISTER FanReadRegisters {
        uint16_t rpm {};
        uint16_t pwm {};
        uint16_t state {};
        uint16_t is_rpm_ok {};
    };

    MODBUS_REGISTER RegisterGeneralStatus_t {
        dwarf_shared::errors::FaultStatusMask FaultStatus {};
        uint16_t HotendMeasuredTemperature {};
        uint16_t HotendPWMState {};
        uint16_t ToolFilamentSensor {};
        uint16_t BoardTemperature {};
        uint16_t MCUTemperature {};
        uint16_t HeatBreakMeasuredTemperature {};
        uint16_t IsPickedRaw {};
        uint16_t IsParkedRaw {};
        FanReadRegisters fan[NUM_FANS] {};
        uint16_t system_24V_mV {};
        uint16_t heater_current_mA {};
    };
    // Cached tool filament sensor value, for use from an interrupt (where we can't lock).
    std::atomic<uint16_t> tool_filament_sensor = 0;

    MODBUS_REGISTER TimeSync_t {
        uint32_t dwarf_time_us {};
    };
    ModbusInputRegisterBlock<TIME_SYNC_ADDR, TimeSync_t> TimeSync {};

    MODBUS_REGISTER GeneralWrite_t {
        uint16_t HotendRequestedTemperature {};
        uint16_t HeatbreakRequestedTemperature {};

        static constexpr uint16_t FAN_AUTO_PWM = std::numeric_limits<uint16_t>::max();
        uint16_t fan_pwm[NUM_FANS] {}; // target PWM or when value is FAN_AUTO_RPM, use automatic control

        struct __attribute__((packed)) {
            uint8_t not_selected {}; // 8 LSb PWM when not selected [0 - 0xff]
            uint8_t selected {}; // 8 MSb PWM when selected [0 - 0xff]
        } led_pwm;

        /// Dwarf status LED control, for encoding see dwarf_shared::StatusLed
        uint16_t status_led[dwarf_shared::StatusLed::REG_SIZE] {};

        struct __attribute__((packed)) {
            float p {};
            float i {};
            float d {};
        } pid;

        uint16_t fan_mode {}; // 0=XL/legacy, 1=XLS native, 2=XLS compat
    };

    // --- Read-side state, populated by read_general_status() / read_discrete_general_status() ---

    // General-status fields, read lock-free from Marlin.
    // HEATER_0_MINTEMP / HEATBREAK_MINTEMP are macros unavailable in this header;
    // the non-zero initial values are set in the constructor instead.
    std::atomic<int16_t> hotend_temp { 0 };
    std::atomic<uint16_t> heater_pwm { 0 };
    std::atomic<int16_t> heatbreak_temp { 0 };
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_pwm {};
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_rpm {};
    std::atomic<uint8_t> fan_rpm_ok { 0 }; // bitmask, one bit per fan
    std::array<std::atomic<uint8_t>, NUM_FANS> fan_state {};
    std::atomic<int16_t> mcu_temperature { 0 };
    std::atomic<int16_t> board_temperature { 0 };
    std::atomic<uint16_t> v24_mV { 0 };
    std::atomic<uint16_t> heater_current_mA { 0 };

    // Discrete-status flags, packed into one byte, read lock-free from Marlin.
    static constexpr uint8_t DISC_PICKED = 1 << 0;
    static constexpr uint8_t DISC_PARKED = 1 << 1;
    static constexpr uint8_t DISC_BTN_UP = 1 << 2;
    static constexpr uint8_t DISC_BTN_DN = 1 << 3;
    std::atomic<uint8_t> discrete_status { 0 };

    /// Gets incremented each time the puppy is reset
    std::atomic<uint32_t> reset_counter { 0 };

    // --- Desired write-side state, applied by write_general() ---

    // Desired hotend target, sent on next write_general().
    std::atomic<uint16_t> hotend_target_temp_desired { 0 };
    // Desired heatbreak target, sent on next write_general().
    // DEFAULT_HEATBREAK_TEMPERATURE is a macro unavailable in this header;
    // the non-zero initial value is set in the constructor instead.
    std::atomic<uint16_t> heatbreak_target_temp_desired { 0 };
    // Desired fan PWMs, sent on next write_general().
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_pwm_desired { 0, 0 };

    // Timestamps for max_age_ms skip logic (puppy task only).
    uint32_t register_general_status_last_read_ms { 0 };
    uint32_t discrete_general_status_last_read_ms { 0 };
    // Atomic so lock-free setters can flip it without taking the mutex.
    std::atomic<bool> general_write_dirty { false };

    // Plain mutex-protected write state for multi-field writes.
    struct __attribute__((packed)) {
        uint8_t not_selected {};
        uint8_t selected {};
    } led_pwm {};

    std::array<uint16_t, dwarf_shared::StatusLed::REG_SIZE> status_led {};

    struct __attribute__((packed)) {
        float p {};
        float i {};
        float d {};
    } pid {};

    // Desired print-fan control mode, sent on next write_general().
    FanMode fan_mode {};

    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<uint8_t>::is_always_lock_free);
    static_assert(std::atomic<uint16_t>::is_always_lock_free);
    static_assert(std::atomic<int16_t>::is_always_lock_free);

    MODBUS_REGISTER TmcWriteRequest_t {
        uint16_t address {};
        uint32_t data {};
    };
    ModbusHoldingRegisterBlock<TMC_WRITE_REQUEST_ADDRESS, TmcWriteRequest_t> TmcWriteRequest {};

    MODBUS_REGISTER TmcReadRequest_t {
        uint16_t address {};
    };
    ModbusHoldingRegisterBlock<TMC_READ_REQUEST_ADDRESS, TmcReadRequest_t> TmcReadRequest {};

    MODBUS_REGISTER TmcReadResponse_t {
        uint32_t value {};
    };
    ModbusInputRegisterBlock<TMC_READ_RESPONSE_ADDRESS, TmcReadResponse_t> TmcReadResponse {};

    ModbusCoil<TMC_ENABLE_ADDR> TmcEnable {};
    ModbusCoil<IS_SELECTED> IsSelectedCoil {};
    ModbusCoil<LOADCELL_ENABLE> LoadcellEnableCoil {};
    ModbusCoil<ACCELEROMETER_ENABLE> AccelerometerEnableCoil {};

    MODBUS_REGISTER MarlinErrorString_t {
        uint16_t title[10] {}; // 20 chars, title of error
        uint16_t message[32] {}; // 64 chars, message of error
    };
    ModbusInputRegisterBlock<MARLIN_ERROR_COMPONENT_START, MarlinErrorString_t> MarlinErrorString {};

private:
    // FIXME: Need to be forward-declared, because this header file is included
    // from marlin and it seems virtually impossible to persuade the **** build
    // system to set the include paths to the place where we hide the
    // freertos/mutex.hpp.
    std::unique_ptr<freertos::Mutex> mutex;

    /// @brief Dwarf number (0-4)
    uint8_t dwarf_number;

    /// @brief Log component asociated with this dwarf
    logging::Component &log_component;

    /// @brief True means this tool is picked and active
    std::atomic<bool> selected;

    // Log transfer buffer and position
    std::array<char, 256> log_line_buffer;
    size_t log_line_pos = 0;
    buddy::puppies::TimeSync time_sync;

    struct LoadcellSamplerate {
        static constexpr float expected = 1000.f / HX717::sample_rate; ///< Expected sampling interval [ms]
        uint32_t count; ///< Number of samples processed in one fifo pull
        uint32_t last_timestamp; ///< Timestamp of last sample
        uint32_t last_processed_timestamp; ///< Timestamp of last update of sampling rate
    } loadcell_samplerate;

    CommunicationStatus write_general(PuppyModbus &);
    CommunicationStatus write_tmc_enable(PuppyModbus &);
    CommunicationStatus pull_fifo_nolock(PuppyModbus &, bool &more);
    bool dispatch_log_event();
    CommunicationStatus run_time_sync(PuppyModbus &);
    constexpr logging::Component &get_log_component(uint8_t dwarf_number);
    CommunicationStatus read_discrete_general_status(PuppyModbus &);
    CommunicationStatus read_general_status(PuppyModbus &);
    void handle_dwarf_fault(PuppyModbus &, dwarf_shared::errors::FaultStatusMask fault_status);
    bool set_loadcell_nolock(PuppyModbus &, bool active);
    bool set_accelerometer_nolock(PuppyModbus &, bool active);
    bool raw_set_loadcell(PuppyModbus &, bool active); // Low level loadcell enable/disable, no dependencies
    bool raw_set_accelerometer(PuppyModbus &, bool active); // Low level accelerometer enable/disable, no dependencies

    // Register refresh control
    uint32_t refresh_nr = 0; ///< Switch of different refresh cases
    uint32_t last_pull_ms = 0; ///< Last time we pulled data from fifo

protected:
    void decode_log(const LogData &data) final;
    void decode_loadcell(const LoadcellRecord &data) final;
    void decode_accelerometer_fast(const AccelerometerFastData &data) final;
    void decode_accelerometer_freq(const AccelerometerSamplingRate &data) final;
};

// we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
extern StrongIndexArray<Dwarf, DWARF_MAX_COUNT, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> dwarfs;

} // namespace buddy::puppies
