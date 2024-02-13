#pragma once

#include "touchscreen_common.hpp"

class Touchscreen_GT911 final : public Touchscreen_Base {

public:
    enum class RecoveryAction {
        none,
        restart_display, ///< This recovery action needs to be performed in cooperation with the display, because display & touchscreen share the reset pin

        // The following actions are handled internally
        disable_touch, ///< [HANDLED INTERNALLY] It simply cannot be done in the ISR, so we have to save state and do it later.
    };

public:
    Touchscreen_GT911();

public:
    void set_enabled(bool set) override;

    /// Touchscreen registers need to be configured upon startup, this function uploads the config.
    /// On success, resets is_hw_ok to true.
    void upload_touchscreen_config();

    /// Checks if the touchscreen unit works properly.
    void perform_check();

    /// Returns what recovery action is needed to be done
    RecoveryAction required_recovery_action() const {
        return required_recovery_action_;
    }

    using ResetClrFunc = void (*)();
    void reset_chip(ResetClrFunc reset_clr_func);

public:
    inline auto total_read_error_count() const {
        return total_read_error_count_;
    }

protected:
    void update_impl(TouchState &touch_state) override;

private:
    static constexpr uint16_t register_esd_check = 0x8041;
    static constexpr uint16_t register_config_begin = 0x8047;
    static constexpr uint16_t register_touch_status = 0x814E;

    // Tried 20 before, was causing touchscreen init problems with USB & bootloader - BFW-5009
    static constexpr int read_write_timeout_ms = 50;
    static constexpr uint16_t i2c_device_address = 0xBA;
    static constexpr int config_data_size = 0x8100 - register_config_begin + 1;

private:
    /// Reads data from the controller EEPROM.
    /// Returns if successful.
    bool read_data(uint16_t address, void *dest, uint8_t bytes, uint8_t attempts = 2);

    /// Writes data to the controller EEPROM.
    /// Returns if successful.
    bool write_data(uint16_t address, const void *data, uint8_t bytes, uint8_t attempts = 2);

    /// Returns whether there is point in trying further attempts
    bool handle_read_error();

private:
    /// Pin state that signalizes there's something to be read from the touch
    buddy::hw::Pin::State touch_sig_read_state_;

private:
    uint32_t total_read_error_count_ = 0;
    uint32_t consecutive_read_error_count_ = 0;
    uint32_t last_update_ms_ = 0;

    /// Timestamp of when the touch event started
    uint32_t touch_start_ms_ = 0;

    /// How many updates there have been since the touch event started
    uint32_t touch_event_update_count_ = 0;

    RecoveryAction required_recovery_action_ = RecoveryAction::none;
};
