///@file
#pragma once

#include "PuppyModbus.hpp"
#include "PuppyBus.hpp"
#include <cstddef>
#include <atomic>
#include <freertos/mutex.hpp>
#include <otp/types.hpp>
#include <puppies/cyphal_flash_host.hpp>
#include <span>
#include <xbuddy_extension/modbus.hpp>
#include <xbuddy_extension/shared_enums.hpp>

namespace buddy::puppies {

class XBuddyExtension final {
public:
    static constexpr size_t FAN_CNT = xbuddy_extension::fan_count;
    using FilamentSensorState = xbuddy_extension::FilamentSensorState;

    // These are called from whatever task that needs them.
    void set_fan_pwm(size_t fan_idx, uint8_t pwm);
    void set_white_led(uint8_t intensity);
    /**
     * Set the strobe frequency of the white led.
     *
     * This overrides the PWM cycle to be "slow" at the given frequency, creating a strobe effect.
     *
     * nullopt leaves it back to the extension board.
     *
     * As the PWM timer is used for some fans too, but it seems their
     * regulation works fine even in that case.
     */
    void set_white_strobe_frequency(std::optional<uint16_t> frequency);
    void set_rgbw_led(std::array<uint8_t, 4> rgbw);
    void set_usb_power(bool enabled);
    void set_mmu_power(bool enabled);
    void set_mmu_nreset(bool enabled);
    std::optional<uint16_t> get_fan_rpm(size_t fan_idx) const;

    /// A convenience function returning a snapshot of all fans' RPMs at once.
    /// Primarily used in feeding the Connect interface with a set of telemetry readings
    /// @returns known measured RPM of all fans at once.
    /// If an data is not valid, returned readings are zeroed - that's what the Connect interface expects
    /// -> no need to play with std::optional which only makes usage much harded.
    std::array<uint16_t, FAN_CNT> get_fans_rpm() const;

    std::optional<float> get_chamber_temp() const;

    /// Single GPIO sensor (PA5 on standard, PA9 on iX)
    std::optional<FilamentSensorState> get_gpio_filament_sensor_state() const;

    /// TMP1826 multi-tool sensor (PC14/EXT connector)
    std::optional<FilamentSensorState> get_ext_filament_sensor_state(uint8_t index) const;

    uint8_t get_requested_fan_pwm(size_t fan_idx);

    /// Get current flash progress (0-100 percent, 0 if not flashing)
    uint8_t get_flash_progress_percent() const;

    bool get_usb_power() const;

    // These are called from the puppy task.
    CommunicationStatus refresh(PuppyModbus &);
    CommunicationStatus initial_scan(PuppyModbus &);
    CommunicationStatus ping(PuppyModbus &);
    CommunicationStatus set_mmu_power(PuppyModbus &, bool mmu_power);

    void set_otp(const OTP_v5 &);
    OTP_v5 get_otp() const;

private:
    // The registers cached here are accessed from different tasks.
    mutable freertos::Mutex mutex;

    // --- Read-side state, populated by refresh_input() ---

    /// If reading/refresh failed, this'll be in invalid state and we'll return
    /// nullopt for queries.
    ///
    /// Used in a lock-like fashion - set to true only after valid values are
    /// published in the status fields below.
    ///
    /// On setting to false, old values are preserved, so any stale check is
    /// just the same as reading it before the valid was set to false.
    std::atomic<bool> valid { false };

    // Fan RPMs from the last status read.
    std::array<std::atomic<uint16_t>, FAN_CNT> fan_rpm {};

    // Chamber temperature (decidegree Celsius).
    std::atomic<uint16_t> chamber_temperature_dc { 0 };

    // GPIO filament sensor state.
    std::atomic<uint16_t> gpio_filament_sensor { 0 };

    // External filament sensors state (2 bits per sensor).
    std::atomic<uint16_t> ext_filament_sensors { 0 };

    // --- Desired write-side state, applied by refresh_holding() ---

    std::array<std::atomic<uint8_t>, FAN_CNT> fan_pwm_desired {};
    std::atomic<uint8_t> w_led_pwm_desired { 0 };
    std::atomic<uint16_t> w_led_frequency_desired { 0 }; // 0 == default / none

    // RGBW components are documented as 0-255 (one byte each) in
    // xbuddy_extension::modbus::Config, so all four pack into one uint32_t.
    // Makes it consistent for atomics. RGBW in order of bytes.
    std::atomic<uint32_t> rgbw_led_desired { 0 };

    std::atomic<bool> usb_power_desired { false };
    std::atomic<bool> mmu_power_desired { false };
    std::atomic<bool> mmu_nreset_desired { false };

    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<uint8_t>::is_always_lock_free);
    static_assert(std::atomic<uint16_t>::is_always_lock_free);
    static_assert(std::atomic<uint32_t>::is_always_lock_free);

    OTP_v5 otp = {};

    using Config = xbuddy_extension::modbus::Config;
    using Status = xbuddy_extension::modbus::Status;

    // Timestamps for stack-built register blocks (puppy task only).
    uint32_t status_last_read_ms { 0 };
    // Atomic so lock-free setters can flip it without taking the mutex.
    std::atomic<bool> config_dirty { false };

    // Plain mutex-protected members populated from status block in refresh_input().
    // Read in refresh_log_message() — sequence number used to detect new logs.
    uint16_t current_log_message_sequence { 0 };

    // Plain puppy-task-only field for the activity heartbeat sent in config.
    uint16_t config_activity { 0 };

    // Track last log sequence to detect new log messages
    uint16_t last_log_message_sequence = 0;

    // To not send activity updates too often.
    uint32_t last_activity_update = 0;

    /// Firmware-flashing host for the pubbies behind the extension board,
    /// driven from refresh(). Guarded by `mutex`.
    CyphalBridgeFlashHost flash;

    CommunicationStatus refresh_holding(PuppyModbus &);
    CommunicationStatus refresh_input(PuppyModbus &, uint32_t max_age);
    CommunicationStatus refresh_log_message(PuppyModbus &);
};

extern XBuddyExtension xbuddy_extension;

} // namespace buddy::puppies
