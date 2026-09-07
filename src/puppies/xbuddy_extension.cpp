#include <puppies/xbuddy_extension.hpp>

#include <puppies/cyphal_flash_files.hpp>
#include "timing.h"
#include <algorithm>
#include <bit>
#include <logging/log.hpp>
#include <modbus/modbus.hpp>
#include <mutex>
#include <utility>
#include <xbuddy_extension/shared_enums.hpp>
#include <puppies/cyphal_bridge.hpp>

LOG_COMPONENT_REF(Buddy);

using Lock = std::unique_lock<freertos::Mutex>;

using FileId = xbuddy_extension::FileId;

static constexpr uint8_t unit = std::to_underlying(modbus::ServerAddress::xbuddy_extension);

namespace {

// The xbuddy takes action if not seen an update for 1 second (in particular,
// it signals its unhealthy status in 1-second heartbeats).
//
// Try to do at least three "activities" during that time, to have some error
// margin if something gets lost or delayed.
constexpr int32_t activity_update_every_ms = 300;

} // namespace

namespace buddy::puppies {

void XBuddyExtension::set_fan_pwm(size_t fan_idx, uint8_t pwm) {
    debug_assert(fan_idx < FAN_CNT);
    if (fan_pwm_desired[fan_idx].exchange(pwm) != pwm) {
        config_dirty.store(true);
    }
}

uint8_t XBuddyExtension::get_requested_fan_pwm(size_t fan_idx) {
    debug_assert(fan_idx < FAN_CNT);
    return fan_pwm_desired[fan_idx].load();
}

uint8_t XBuddyExtension::get_flash_progress_percent() const {
    Lock lock(mutex);
    return flash.progress_percent();
}

bool XBuddyExtension::get_usb_power() const {
    return usb_power_desired.load();
}

void XBuddyExtension::set_white_led(uint8_t intensity) {
    if (w_led_pwm_desired.exchange(intensity) != intensity) {
        config_dirty.store(true);
    }
}

void XBuddyExtension::set_white_strobe_frequency(std::optional<uint16_t> freq) {
    debug_assert(freq != 0); // Explicit 0 makes no sense as frequency
    const uint16_t value = freq.value_or(0);
    if (w_led_frequency_desired.exchange(value) != value) {
        config_dirty.store(true);
    }
}

void XBuddyExtension::set_rgbw_led(std::array<uint8_t, 4> color) {
    const uint32_t packed = static_cast<uint32_t>(color[0])
        | (static_cast<uint32_t>(color[1]) << 8)
        | (static_cast<uint32_t>(color[2]) << 16)
        | (static_cast<uint32_t>(color[3]) << 24);
    if (rgbw_led_desired.exchange(packed) != packed) {
        config_dirty.store(true);
    }
}

void XBuddyExtension::set_usb_power(bool enabled) {
    if (usb_power_desired.exchange(enabled) != enabled) {
        config_dirty.store(true);
    }
}

void XBuddyExtension::set_mmu_power(bool enabled) {
    if (mmu_power_desired.exchange(enabled) != enabled) {
        config_dirty.store(true);
    }
}

void XBuddyExtension::set_mmu_nreset(bool enabled) {
    if (mmu_nreset_desired.exchange(enabled) != enabled) {
        config_dirty.store(true);
    }
}

std::optional<uint16_t> XBuddyExtension::get_fan_rpm(size_t fan_idx) const {
    debug_assert(fan_idx < FAN_CNT);
    if (!valid.load()) {
        return std::nullopt;
    }
    return fan_rpm[fan_idx].load();
}

std::array<uint16_t, XBuddyExtension::FAN_CNT> XBuddyExtension::get_fans_rpm() const {
    if (!valid.load()) {
        return std::array<uint16_t, FAN_CNT> { 0, 0, 0 };
    }
    std::array<uint16_t, FAN_CNT> out;
    for (size_t i = 0; i < FAN_CNT; ++i) {
        out[i] = fan_rpm[i].load();
    }
    return out;
}

std::optional<float> XBuddyExtension::get_chamber_temp() const {
    if (!valid.load()) {
        return std::nullopt;
    }
    return static_cast<float>(chamber_temperature_dc.load()) / 10.0f;
}

std::optional<XBuddyExtension::FilamentSensorState> XBuddyExtension::get_gpio_filament_sensor_state() const {
    if (!valid.load()) {
        return std::nullopt;
    }
    return static_cast<FilamentSensorState>(gpio_filament_sensor.load());
}

std::optional<XBuddyExtension::FilamentSensorState> XBuddyExtension::get_ext_filament_sensor_state(uint8_t index) const {
    if (!valid.load()) {
        return std::nullopt;
    }
    using Register = decltype(Status::ext_filament_sensors);
    debug_assert(index < xbuddy_extension::ext_filament_sensor_count);
    const uint8_t shift = index * xbuddy_extension::bits_per_fs_state;
    constexpr Register mask = (Register(1) << xbuddy_extension::bits_per_fs_state) - 1;
    return static_cast<FilamentSensorState>((ext_filament_sensors.load() >> shift) & mask);
}

CommunicationStatus XBuddyExtension::refresh_input(PuppyModbus &bus, uint32_t max_age) {
    // Already locked by caller.

    ModbusInputRegisterBlock<Status::address, Status> block {};
    block.last_read_timestamp_ms = status_last_read_ms;
    const CommunicationStatus result = bus.read(unit, block, max_age);
    status_last_read_ms = block.last_read_timestamp_ms;

    switch (result) {
    case CommunicationStatus::OK:
        for (size_t i = 0; i < FAN_CNT; ++i) {
            fan_rpm[i].store(block.value.fan_rpm[i]);
        }
        chamber_temperature_dc.store(block.value.temperature);
        gpio_filament_sensor.store(block.value.gpio_filament_sensor);
        ext_filament_sensors.store(block.value.ext_filament_sensors);
        current_log_message_sequence = block.value.log_message_sequence;
        flash.set_requests(block.value.chunk_request, block.value.digest_request);
        // Only after publishing all status fields, to make sure to never
        // expose an invalid value.
        valid.store(true);
        break;
    case CommunicationStatus::ERROR:
        valid.store(false);
        flash.invalidate();
        break;
    default:
        // SKIPPED doesn't change the validity.
        break;
    }

    return result;
}

CommunicationStatus XBuddyExtension::refresh_holding(PuppyModbus &bus) {
    // Already locked by caller

    // Clear dirty before snapshotting; a racing setter then either lands in
    // our snapshot or re-marks dirty for the next cycle.
    bool dirty = config_dirty.exchange(false);

    const uint32_t now = ticks_ms();
    // Update activity every time so it stays fresh; mark dirty only periodically
    // so we piggy-back on other requests and avoid unnecessary round trips.
    config_activity = static_cast<uint16_t>(now);
    if (ticks_diff(now, last_activity_update) > activity_update_every_ms) {
        dirty = true;
    }

    ModbusHoldingRegisterBlock<Config::address, Config> block {};
    for (size_t i = 0; i < FAN_CNT; ++i) {
        block.value.fan_pwm[i] = static_cast<uint16_t>(fan_pwm_desired[i].load());
    }
    block.value.w_led_pwm = static_cast<uint16_t>(w_led_pwm_desired.load());
    block.value.w_led_frequency = w_led_frequency_desired.load();
    const uint32_t rgbw_snap = rgbw_led_desired.load();
    block.value.rgbw_led_r_pwm = static_cast<uint16_t>(rgbw_snap & 0xff);
    block.value.rgbw_led_g_pwm = static_cast<uint16_t>((rgbw_snap >> 8) & 0xff);
    block.value.rgbw_led_b_pwm = static_cast<uint16_t>((rgbw_snap >> 16) & 0xff);
    block.value.rgbw_led_w_pwm = static_cast<uint16_t>((rgbw_snap >> 24) & 0xff);
    block.value.usb_power = static_cast<uint16_t>(usb_power_desired.load() ? 1 : 0);
    block.value.mmu_power = static_cast<uint16_t>(mmu_power_desired.load() ? 1 : 0);
    block.value.mmu_nreset = static_cast<uint16_t>(mmu_nreset_desired.load() ? 1 : 0);
    block.value.activity = config_activity;
    block.dirty = dirty;

    const CommunicationStatus result = bus.write(unit, block);
    if (result == CommunicationStatus::ERROR && dirty) {
        // Write didn't go through, keep work for next cycle.
        config_dirty.store(true);
    }

    if (result == CommunicationStatus::OK) {
        last_activity_update = now;
    }

    return result;
}

CommunicationStatus XBuddyExtension::refresh(PuppyModbus &bus) {
    Lock lock(mutex);

    return aggregate_communication_status({
        // Refresh on every exchange in case we are flashing - we want to update
        // the request ASAP, it's changing after each sent chunk.
        refresh_input(bus, flash.flashing() ? 0 : 250),
        refresh_holding(bus),
        refresh_log_message(bus),
        flash.write_chunk(bus, modbus::ServerAddress::xbuddy_extension),
        flash.write_digest(bus, modbus::ServerAddress::xbuddy_extension, [&lock](xbuddy_extension::modbus::DigestRequest request, FileId file_id, xbuddy_extension::modbus::Digest &out) {
            // Release mutex for the slow digest computation (file read + SHA256).
            lock.unlock();
            compute_digest_response(request, file_id, out);
            lock.lock();
        }),
        cyphal_bridge.refresh(bus, modbus::ServerAddress::xbuddy_extension),
    });
}

CommunicationStatus XBuddyExtension::initial_scan(PuppyModbus &bus) {
    Lock lock(mutex);

    const auto input = refresh_input(bus, 0);
    config_dirty.store(true);
    return input;
}

CommunicationStatus XBuddyExtension::ping(PuppyModbus &bus) {
    Lock lock(mutex);

    return refresh_input(bus, 0);
}

CommunicationStatus XBuddyExtension::set_mmu_power(PuppyModbus &bus, bool mmu_power) {
    // Called from PuppyBootstrap; synchronous flush instead of deferring to the next poll cycle.
    Lock lock(mutex);
    if (mmu_power_desired.exchange(mmu_power) != mmu_power) {
        config_dirty.store(true);
    }
    return refresh_holding(bus);
}

CommunicationStatus XBuddyExtension::refresh_log_message(PuppyModbus &bus) {
    // Already locked by caller

    // Check if log_message_sequence changed at all
    if (current_log_message_sequence == last_log_message_sequence) {
        return CommunicationStatus::SKIPPED;
    }

    xbuddy_extension::modbus::LogMessage log_message;
    if (!bus.read_input_registers(modbus::ServerAddress::xbuddy_extension, log_message)) {
        // Do not update last_log_message_sequence, it will be retried on next cycle
        log_warning(Buddy, "XBE: failed to read log message");
        return CommunicationStatus::ERROR;
    }

    // Check if we missed any messages, work in modular arithmetic
    const uint16_t expected_sequence = static_cast<uint16_t>(last_log_message_sequence + 1);
    if (log_message.sequence != expected_sequence) {
        // If we missed message, we at least log...
        log_info(Buddy, "XBE: missed log message(s)");
        // ...and continue with the newest message.
    }

    static_assert(std::endian::native == std::endian::little);
    const auto text_size = std::min((size_t)log_message.text_size, 2 * log_message.text_data.size());
    const auto text_data = (const char *)log_message.text_data.data();
    log_info(Buddy, "XBE: %.*s", text_size, text_data);

    last_log_message_sequence = log_message.sequence;
    return CommunicationStatus::OK;
}

void XBuddyExtension::set_otp(const OTP_v5 &otp_data) {
    Lock lock(mutex);
    otp = otp_data;
}

OTP_v5 XBuddyExtension::get_otp() const {
    Lock lock(mutex);
    return otp;
}

XBuddyExtension xbuddy_extension;

} // namespace buddy::puppies
