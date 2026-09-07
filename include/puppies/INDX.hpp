#pragma once

#include <limits>
#include <array>
#include <atomic>
#include <memory>
#include <optional>

#include <indx_head/modbus.hpp>

#include <otp/types.hpp>
#include "puppies/PuppyModbus.hpp"
#include <fifo_coder/fifo_decoder.hpp>
#include "puppies/time_sync.hpp"
#include <include/dwarf_registers.hpp>
#include <utils/utility_extensions.hpp>
#include <puppies/dwarf_status_led.hpp>
#include <indx_head/errors.hpp>
#include <indx_head/leds.hpp>
#include <utils/color.hpp>
#include <timing.h>

namespace freertos {
class Mutex;
}

using namespace fifo_coder;

namespace buddy::puppies {

class Indx final : public ModbusDevice, public Decoder::Callbacks {
public:
    using SystemCoil = dwarf_shared::registers::SystemCoil;
    using SystemFIFO = dwarf_shared::registers::SystemFIFO;

    static constexpr uint16_t ENCODED_FIFO_ADDRESS { std::to_underlying(SystemFIFO::encoded_stream) };

    static constexpr uint_fast8_t NUM_FANS = 2;
    static constexpr uint8_t FIFO_RETRIES = 3;

    /// when this is set as PWM, fan is switched to automatic mode
    static constexpr uint16_t FAN_MODE_AUTO_PWM = std::numeric_limits<uint16_t>::max();

public:
    Indx(uint8_t modbus_address);
    Indx(const Indx &) = delete;

    CommunicationStatus ping(PuppyModbus &);
    CommunicationStatus initial_scan(PuppyModbus &);

    /**
     * @brief Refreshes all registers from dwarf.
     * @return CommunicationStatus::OK on successful refresh and
     *   CommunicationStatus::SKIPPED on successful skip.
     */
    CommunicationStatus refresh(PuppyModbus &);

    /**
     * @brief Pulls data from dwarf fifo.
     * Originally fast_refresh().
     * @param[out] more true if there is more data to pull, false if fifo is empty
     * @return CommunicationStatus::OK on success
     */
    CommunicationStatus pull_fifo(PuppyModbus &, bool &more);

    /**
     * @brief Asynchronously enable/disable loadcell data in FIFO.
     *
     * The puppy task performs the write on its next general-write pass.
     * @param active Enable loadcell FIFO output
     */
    void set_loadcell(bool active);

    /**
     * @brief Get loadcell active state
     *
     * @return True if loadcell output is enabled, false otherwise
     */
    bool get_loadcell_active();

    /// True while a general write has been requested but not yet flushed by the puppy task.
    bool is_write_pending() const { return general_write_dirty.load(); }

    /**
     * @brief Enable/disable accelerometer data in FIFO.
     *
     * @param active Enable accelerometer FIFO output
     * @return True when successful, false on communication error
     */
    bool set_accelerometer(PuppyModbus &, bool active);

    /**
     * @brief Gets accelerometer active state
     *
     * @return True if accelerometer output is enabled, false otherwise
     */
    bool get_accelerometer_active();

    CommunicationStatus set_hotend_target_temp(float target);

    /// Nozzle target temp [°C] last sent to the head; 0 while parked / no tool (nonzero => actively heating).
    [[nodiscard]] uint16_t get_hotend_target_temp() const { return nozzle_target_temperature_desired.load(); }

    CommunicationStatus set_hotend_temp_compensation(float offset);
    [[nodiscard]] float get_hotend_temp_compensated() const;
    [[nodiscard]] float get_hotend_temp_uncompensated() const;

    /// In °C/s
    [[nodiscard]] float get_hotend_temp_raw_c_dt_s() const;

    /// @returns whether get_nozzle_temp_uncompensated_c100, and get_tpis_ambient_temp_c100 contain valid values instead of initial garbage
    /// Once the temps get valid, they can only become invalid if the puppy is reset.
    /// Read before the get_temp_XX to avoid race conditions.
    [[nodiscard]] bool get_temps_valid() const {
        return temps_valid.load();
    }

    /// PWM (0-255) representing power flowing to nozzle
    [[nodiscard]] uint8_t get_hotend_pwm_averaged() const;

    /// Integral of measured V × I power [uJ]
    /// !!! Overflows periodically (~71s at 60W)
    [[nodiscard]] uint32_t get_hotend_energy_consumed_uJ() const;

    /// Fraction of the coil power delivered to nozzle
    /// Relates to get_hotend_energy_consumed_uJ
    static constexpr float hotend_induction_efficiency = 0.94f;

    [[nodiscard]] int16_t get_mcu_temperature(); ///< Get MCU temperature [°C]
    [[nodiscard]] int16_t get_board_temperature(); ///< Get board temperature [°C]
    [[nodiscard]] float get_tpis_ambient_temperature(); ///< Get TPiS sensor ambient temperature [°C]
    [[nodiscard]] int16_t get_ringdown_decay() const; ///< Latest ringdown analysis decay × 1000 (unitless); 0 when the analysis failed
    [[nodiscard]] uint16_t get_heater_current_mA() const; ///< Last-sampled induction heater coil current [mA]; reads ~0 when not heating
    [[nodiscard]] float get_24V(); ///< Get 24V power supply voltage [V]
    /** Get nozzle presence (debounced on the INDX_HEAD side).
     *  @returns nullopt until the head reports a definitive value, true if nozzle is present, false otherwise
     */
    [[nodiscard]] std::optional<bool> get_nozzle_present();
    void invalidate_nozzle_data(); ///< Invalidate after pickup/park

    /// @returns ticks_ms of the last successful read of register_general_status
    std::optional<uint32_t> get_register_general_status_last_read_ms() const;

    void set_fan(uint8_t fan, uint16_t target);
    void set_fan_auto(uint8_t fan);
    void set_selftest_mode(bool enabled);

    /**
     * @brief Set INDX_HEAD LEDs' color.
     * @param color
     * @param mode set up led pwm mode
     */
    void set_leds_solid_color(Color color, uint16_t delay_ms = 0);

    void set_leds_blinking(Color primary, Color secondary, uint16_t delay_ms);
    void set_leds_pulsing(Color primary, Color secondary, uint16_t delay_ms);
    void set_leds_to_follow_nozle_temp();

    /**
     * @brief Power INDX_HEAD LED on/off.
     */
    void set_leds_enabled(bool set);

    uint16_t get_heatbreak_fan_pwr();

    uint16_t get_fan_pwm(uint8_t fan_nr) const;
    uint16_t get_fan_rpm(uint8_t fan_nr) const;
    bool get_fan_rpm_ok(uint8_t fan_nr) const;
    uint16_t get_fan_state(uint8_t fan_nr) const;

    void set_otp(const OTP_v5 &);
    OTP_v5 get_otp() const;

    /// @returns a value that increments every time the puppy is reset
    /// It is recommended to check for the reset counter against the previous one
    /// AFTER reading out all the needed data from the puppy.
    [[nodiscard]] uint32_t get_reset_counter() const {
        return reset_counter.load();
    }

private:
    OTP_v5 otp = {};

    /// Cached nozzle presence for use by Marlin.
    ///
    /// (encodes validity too).
    std::atomic<indx_head::NozzlePresence> nozzle_state { indx_head::NozzlePresence::unknown };
    static_assert(std::atomic<indx_head::NozzlePresence>::is_always_lock_free);

    std::atomic<uint16_t> nozzle_invalidation_token { 0 }; ///< Token sent to head; data is valid only after head echoes it back nozzle_invalidation_ack from INDX_HEAD

    // Hotend temperature fields — populated from read_general_status(), read lock-free by Marlin.
    std::atomic<int16_t> hotend_temp_compensated_c100 { indx_head::modbus::default_hotend_temperature_c100 };
    std::atomic<int16_t> hotend_temp_uncompensated_c100 { indx_head::modbus::default_hotend_temperature_c100 };
    std::atomic<int16_t> hotend_temp_raw_c100_dt_s { 0 };
    std::atomic<uint8_t> hotend_pwm_averaged { 0 };
    std::atomic<uint32_t> hotend_energy_consumed_uJ { 0 };
    std::atomic<bool> temps_valid { false };

    // General-status fields — populated by read_general_status(), read lock-free from Marlin.
    std::atomic<int16_t> mcu_temperature { 0 };
    std::atomic<int16_t> board_temperature { 0 };
    std::atomic<int16_t> tpis_ambient_temperature_c100 { 0 };
    std::atomic<uint16_t> v24_mV { 0 };
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_pwm {};
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_rpm {};
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_state {};
    std::atomic<uint8_t> fan_rpm_ok { 0 }; // bitmask: bit 0 = print fan, bit 1 = heatbreak fan

    std::atomic<int16_t> ringdown_decay { 0 };
    std::atomic<uint16_t> heater_current_mA { 0 };

    static_assert(std::atomic<int16_t>::is_always_lock_free);
    static_assert(std::atomic<uint16_t>::is_always_lock_free);
    static_assert(std::atomic<uint8_t>::is_always_lock_free);

    // Desired values for temperature control — written lock-free by Marlin, applied in write_general().
    std::atomic<uint16_t> nozzle_target_temperature_desired { 0 };
    std::atomic<int16_t> hotend_temperature_compensation_c100_desired { 0 };

    // Because they can be set from an interrupt.
    std::array<std::atomic<uint16_t>, NUM_FANS> fan_pwm_desired { 0, 0 };
    std::atomic<bool> selftest_mode_ { false };

    /// ticks_ms of the last successful read of register_general_status
    /// 0 = not read at all yet. 0 is guaranteed to never happen afterwards (-1 is used instead to avoid conflict)
    /// Gets reset to 0 when the puppy is reset
    std::atomic<uint32_t> register_general_status_last_read_ms { 0 };

    /// Gets incremented each time the puppy is reset
    std::atomic<uint32_t> reset_counter { 0 };

    // Internal max_age_ms skip timestamp for register_general_status (puppy task only).
    uint32_t register_general_status_modbus_last_read_ms { 0 };
    // Atomic so lock-free setters can flip it without taking the mutex.
    std::atomic<bool> general_write_dirty { false };

    // Plain mutex-protected write state for multi-field writes.
    indx_head::leds::LedConfig leds {};
    indx_head::leds::Mode desired_led_mode = indx_head::leds::Mode::solid;

    void set_leds_config(indx_head::leds::Mode mode, Color primary = Color::from_rgb(0, 0, 0), Color secondary = Color::from_rgb(0, 0, 0), uint16_t delay_ms = 0);

    bool loadcell_enabled { false };
    bool accelerometer_enabled { false };
    /// One-shot fault acknowledgment: set to fault mask, flushed by write_general(), reset to 0 after success.
    uint16_t clear_fault_status_pending { 0 };

    // FIXME: Need to be forward-declared, because this header file is included
    // from marlin and it seems virtually impossible to persuade the **** build
    // system to set the include paths to the place where we hide the
    // freertos/mutex.hpp.
    std::unique_ptr<freertos::Mutex> mutex;

    buddy::puppies::TimeSync time_sync;

    struct LoadcellSamplerate {
        static constexpr float loadcell_sample_rate = 366.f; ///< Sample rate from INDX_HEAD
        static constexpr float expected = 1000.f / loadcell_sample_rate; ///< Expected sampling interval [ms]
        uint32_t count; ///< Number of samples processed in one fifo pull
        uint32_t last_timestamp; ///< Timestamp of last sample
        uint32_t last_processed_timestamp; ///< Timestamp of last update of sampling rate
    } loadcell_samplerate;

    CommunicationStatus write_general(PuppyModbus &);
    bool dispatch_log_event();
    CommunicationStatus read_general_status(PuppyModbus &);
    void handle_fault_status(indx_head::errors::FaultStatusMask fault);
    void handle_nozzle_presence(uint16_t nozzle_present, uint16_t nozzle_invalidation_ack); ///< Update nozzle_state
    void handle_time_sync(uint32_t time_sync_hi, uint32_t time_sync_lo, const RequestTiming &);

    // Register refresh control
    uint32_t refresh_nr = 0; ///< Switch of different refresh cases
    uint32_t last_pull_ms = 0; ///< Last time we pulled data from fifo

protected:
    void decode_log(const LogData &data) final;
    void decode_loadcell(const LoadcellRecord &data) final;
    void decode_accelerometer_fast(const AccelerometerFastData &data) final;
    void decode_accelerometer_freq(const AccelerometerSamplingRate &data) final;
};

extern Indx indx;

} // namespace buddy::puppies
