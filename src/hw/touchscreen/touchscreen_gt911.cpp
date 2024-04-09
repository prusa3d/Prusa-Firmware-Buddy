#include "touchscreen_gt911.hpp"
#include "i2c.hpp"
#include "touchscreen_gt911_config.hpp"

#include "screen_home.hpp"
#include "hw_configuration.hpp"
#include "scope_guard.hpp"

#define CONCAT_IMPL(a, b) a /**/##/**/ b
#define CONCAT(a, b)      CONCAT_IMPL(a, b)

Touchscreen_GT911::Touchscreen_GT911() {
    touch_sig_read_state_ = buddy::hw::Configuration::Instance().has_inverted_touch_interrupt() ? buddy::hw::Pin::State::low : buddy::hw::Pin::State::high;
}

void Touchscreen_GT911::set_enabled(bool set) {
    if (set == is_enabled()) {
        return;
    }

    // Force full setup on re-enabling touchscreen
    if (set) {
        required_recovery_action_ = RecoveryAction::restart_display;
    }

    Touchscreen_Base::set_enabled(set);
}

void Touchscreen_GT911::upload_touchscreen_config() {
    static_assert(sizeof(touchscreen_gt911_config) == config_data_size, "wrong size of config");
    assert(required_recovery_action_ == RecoveryAction::none);

    is_hw_ok_ = false;

    const auto update_config = [&] {
        uint8_t cfg[config_data_size];

        // Download config from the controller
        if (!read_data(register_config_begin, cfg, config_data_size, 5)) {
            metric_record_string(metric_touch_event(), "config read fail");
            log_info(Touch, "config read failed");
            return false;
        }

        log_info(Touch, "config read ok");

        // If the config is the newest, we don't need to flash a new one -> finished
        if (!memcmp(cfg, touchscreen_gt911_config, config_data_size - 2)) {
            metric_record_string(metric_touch_event(), "config no change");
            log_info(Touch, "config no change");
            return true;
        }

        log_info(Touch, "needs new config");

        // Otherwise prepare new data do be flashed
        memcpy(cfg, touchscreen_gt911_config, config_data_size - 2);

        cfg[config_data_size - 1] = 1; // enforce update flag

        // Calc config checksum
        // (not needed when setting from default data, just to be safe)
        {
            uint8_t config_checksum = 0;
            for (uint8_t *it = cfg, *end = cfg + config_data_size - 2; it != end; it++) {
                config_checksum += *it;
            }
            // ccsum %= 256;
            config_checksum = (~config_checksum) + 1;
            cfg[config_data_size - 2] = config_checksum;
        }

        if (!write_data(register_config_begin, cfg, config_data_size)) {
            metric_record_string(metric_touch_event(), "config write failed");
            log_info(Touch, "config write failed");
            return false;
        }

        metric_record_string(metric_touch_event(), "new config flashed");
        log_info(Touch, "new config flashed");
        return true;
    };
    if (!update_config()) {
        return;
    }

    // Enable ESD protection
    {
        const uint8_t data = 0;
        write_data(0x8041, &data, 1);
    }

    is_hw_ok_ = true;
}

void Touchscreen_GT911::perform_check() {
    if (!is_hw_ok_) {
        return;
    }

    // Do not perform checks until home screen has been at least once opened
    // things don't work properly before that for whatever reason
    if (!screen_home_data_t::EverBeenOpened()) {
        return;
    }

    // Try performing recovery
    {
        switch (required_recovery_action_) {

        case RecoveryAction::none:
            break;

        case RecoveryAction::restart_display:
            // Has to be handled externally
            return;

        case RecoveryAction::disable_touch:
            metric_record_string(metric_touch_event(), "disable_touch");
            log_error(Touch, "Touch error, disabling touch");

            set_enabled(false);
            consecutive_read_error_count_ = 0;
            screen_home_data_t::SetTouchBrokenDuringRun();
            break;
        }

        required_recovery_action_ = RecoveryAction::none;
    }

    uint8_t data;
    if (!read_data(register_config_begin, &data, 1)) {
        // Error handling resolved inside read_data

    } else if (data != touchscreen_gt911_config[0]) {
        log_debug(Touch, "Read config differs");
        handle_read_error();

    } else {
        consecutive_read_error_count_ = 0;
    }
}

void Touchscreen_GT911::reset_chip(ResetClrFunc reset_clr_func) {
    using namespace buddy::hw;

    {
        OutputEnabler touch_sig_output(touch_sig, Pin::State::low, OMode::pushPull, OSpeed::low);

        // This reset procedure is (probably) according to GT911 Programming Guide: Timing for host resetting GT911

        /* T1: > 100us */
        delay_us(110);

        /* begin select I2C slave addr */

        /* HIGH: 0x28/0x29 (0x14 7bit), LOW: 0xBA/0xBB (0x5D 7bit) */
        touch_sig_output.write(Pin::State::low);

        /* T2: > 100us */
        delay_us(110);

        reset_clr_func();

        /* T3: > 5ms */
        delay_ms(6);

        touch_sig_output.write(Pin::State::low);
        /* end select I2C slave addr */

        /* T4: 50ms */
        delay_ms(51);
    }

    required_recovery_action_ = RecoveryAction::none;
}

bool Touchscreen_GT911::read_data(uint16_t address, void *dest, uint8_t bytes, uint8_t attempts) {
    if (static_cast<bool>(required_recovery_action_)) {
        return false;
    }

    for (uint8_t attempt = 0; attempt < attempts; attempt++) {
        if (HAL_I2C_Mem_Read(&I2C_HANDLE_FOR(touch), i2c_device_address, address, I2C_MEMADD_SIZE_16BIT, reinterpret_cast<uint8_t *>(dest), bytes, read_write_timeout_ms) == HAL_StatusTypeDef::HAL_OK) {
            return true;
        }

        if (!handle_read_error()) {
            return false;
        }
    }

    return false;
}

bool Touchscreen_GT911::write_data(uint16_t address, const void *data, uint8_t bytes, uint8_t attempts) {
    if (static_cast<bool>(required_recovery_action_)) {
        return false;
    }

    for (uint8_t attempt = 0; attempt < attempts; attempt++) {
        if (HAL_I2C_Mem_Write(&I2C_HANDLE_FOR(touch), i2c_device_address, address, I2C_MEMADD_SIZE_16BIT, reinterpret_cast<uint8_t *>(const_cast<void *>(data)), bytes, read_write_timeout_ms) == HAL_StatusTypeDef::HAL_OK) {
            return true;
        }

        log_warning(Touch, "touch write error");
        metric_record_string(metric_touch_event(), "write_error");
    }

    return false;
}

void Touchscreen_GT911::update_impl(TouchState &touch_state) {
    // This function can be called from IRQ – so in the middle of some thread doing stuff with the I2C.
    // This was actually happening - BFW-5054.
    // So if the I2C is being used, simply do not update and try it later
    if (
        // Check if we're in the ISR
        __get_IPSR()
        // And this ugly hack is to check if the mutex is locked...
        && !uxQueueMessagesWaitingFromISR(i2c::ChannelMutex::get_handle(I2C_HANDLE_FOR(touch)))) {
        return;
    }

    const uint32_t now = ticks_ms();
    const auto diff = ticks_diff(now, last_update_ms_);

    const bool touch_sig_detected = buddy::hw::touch_sig.read() != touch_sig_read_state_;
    if ((touch_sig_detected && !config_store().touch_sig_workaround.get()) || diff < 10) {
        return;
    }

    last_update_ms_ = now;

    struct __attribute__((__packed__)) TouchPointData_GT911 {
        uint8_t trackId;
        uint16_t y_position;
        uint16_t x_position;
        uint16_t area;
        uint8_t reserved;
    };
    static_assert(sizeof(TouchPointData_GT911) == 8);

    // We don't need to work with more than 1 touch point for now
    static constexpr uint8_t max_touch_point_count = 1;

    // Read the touch points data from the touchscreen
    TouchPointData_GT911 touch_points[max_touch_point_count];
    int8_t touch_point_count = 0;
    {
        uint8_t touch_status;
        if (!read_data(register_touch_status, &touch_status, 1)) {
            return;
        }

        if (!(touch_status & 0x80)) {
            log_debug(Touch, "Touch error - touch status register expected to have the 0x80 flag");
            // metric_record_string(metric_touch_event(), "invalid_touch_status %i", touch_status);
            return;
        }

        /// Clear the status register - !!! must be done after reading all the data
        ScopeGuard scope_clear_touch_status = [&]() {
            const uint8_t data = 0;
            write_data(register_touch_status, &data, 1);
        };

        touch_event_update_count_++;

        touch_point_count = std::min<uint8_t>((touch_status & 0xf), max_touch_point_count);
        if (touch_point_count > 0 && !touch_start_ms_) {
            touch_start_ms_ = now;

        } else if (touch_point_count == 0 && touch_start_ms_) {
            const auto touch_duration_ms = ticks_diff(now, touch_start_ms_);

            // Detection code for ghost touch events caused by EMC
            // These values were empirically sucked out of my thumb as a part of BFW-5073
            if (
                (touch_duration_ms < 15) || //
                (touch_duration_ms > 15 && touch_duration_ms < 35 && touch_event_update_count_ != 4) //
            ) {
                // Invalidate the current exture
                touch_state.invalidate = true;

                // Stop receiving touch events for some time
                last_update_ms_ = now + 5000;

                log_info(Touch, "EMC DETECTED TDUR %li UPD %li", touch_duration_ms, touch_event_update_count_);
                metric_record_string(metric_touch_event(), "emc_detected");
                metric_record_string(metric_touch_event(), "emc_detected td=%li uc=%li", touch_duration_ms, touch_event_update_count_);
            }
            // log_info(Touch, "TDUR %i UPD %i", touch_duration_ms, touch_event_update_count_);

            touch_start_ms_ = 0;
            touch_event_update_count_ = 0;
        }

        touch_state.multitouch_point_count = touch_point_count;

        if (touch_point_count < 0) {
            handle_read_error();
            return;
        } else if (touch_point_count == 0) {
            return;
        }

        log_debug(Touch, "Touch count %d", touch_point_count);

        static_assert(sizeof(touch_points) == sizeof(TouchPointData_GT911) * max_touch_point_count);
        if (!read_data(register_touch_status + 1, touch_points, touch_point_count * sizeof(TouchPointData_GT911))) {
            return;
        }
    }

    // Report that the touch sig workaround is on and we've detected a touch data without the interrupt
    if (!touch_sig_detected) {
        metric_record_string(metric_touch_event(), "touch_no_sig");
    }

    // Translate the raw data into the touch_state struct
    for (int i = 0; i < touch_point_count; i++) {
        const auto &tp = touch_points[i];

        touch_state.multitouch_points[i] = TouchPointData {
            .position = { .x = tp.x_position, .y = tp.y_position },
        };
    }
}

bool Touchscreen_GT911::handle_read_error() {
    static constexpr uint32_t retries_before_reset = 8; // how many times must touch read fail to be considered an error
    static constexpr uint32_t retries_before_disable = 4; // if 4 resets did not help, just disable it, home screen will show msgbox

    // If display reset is queued, do not increase errors until it is restarted
    if (static_cast<bool>(required_recovery_action_)) {
        return false;
    }

    metric_record_string(metric_touch_event(), "read_error");

    ++consecutive_read_error_count_;
    ++total_read_error_count_;

    // If we've tried too many times, just disable the touch
    if (consecutive_read_error_count_ > (retries_before_disable * retries_before_reset)) {
        log_warning(Touch, "Touch error, requesting disable");
        required_recovery_action_ = RecoveryAction::disable_touch;
    }

    // Before that, try restarting the display a few times
    else if ((consecutive_read_error_count_ % retries_before_reset) == 0) {
        log_warning(Touch, "Touch error, requesting display restarts");
        metric_record_string(metric_touch_event(), "request_restart_display");
        required_recovery_action_ = RecoveryAction::restart_display;
    }

    // And even before that, try restarting just the I2C
    else {
        log_warning(Touch, "Touch error, restarting I2C");
        metric_record_string(metric_touch_event(), "restart_i2c");
        i2c::ChannelMutex _anym(I2C_HANDLE_FOR(touch));
        HAL_I2C_DeInit(&CONCAT(hi2c, i2c_touch));
        I2C_INIT(touch);
    }

    return true;
}
