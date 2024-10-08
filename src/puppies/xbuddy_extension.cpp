#include <puppies/xbuddy_extension.hpp>
#include <puppies/PuppyBootstrap.hpp>
#include <cassert>

using Lock = std::unique_lock<freertos::Mutex>;

namespace buddy::puppies {

XBuddyExtension::XBuddyExtension(PuppyModbus &bus, uint8_t modbus_address)
    : ModbusDevice(bus, modbus_address) {}

void XBuddyExtension::set_fan_pwm(size_t fan_idx, uint8_t pwm) {
    Lock lock(mutex);

    assert(fan_idx < FAN_CNT);

    if (requirement.value.fan_pwm[fan_idx] != pwm) {
        requirement.value.fan_pwm[fan_idx] = pwm;
        requirement.dirty = true;
    }
}

void XBuddyExtension::set_white_led(uint8_t intensity) {
    Lock lock(mutex);

    if (requirement.value.white_led != intensity) {
        requirement.value.white_led = intensity;
        requirement.dirty = true;
    }
}

void XBuddyExtension::set_rgbw_led(std::array<uint8_t, 4> color) {
    Lock lock(mutex);

    bool same = true;

    // A cycle, because uint8_t vs uint16_t
    for (size_t i = 0; i < 4; i++) {
        if (color[i] != requirement.value.rgbw_led[i]) {
            same = false;
        }
    }

    if (same) {
        return;
    }

    for (size_t i = 0; i < 4; i++) {
        requirement.value.rgbw_led[i] = color[i];
    }

    requirement.dirty = true;
}

std::optional<uint16_t> XBuddyExtension::get_fan_rpm(size_t fan_idx) {
    Lock lock(mutex);

    assert(fan_idx < FAN_CNT);

    if (!valid) {
        return std::nullopt;
    }

    // For some arcane C++ reasons, it doesn't compile with just return
    // status.value..., so the intermediate variable.
    const uint16_t result = status.value.fan_rpm[fan_idx];
    return result;
}

std::optional<float> XBuddyExtension::get_chamber_temp() {
    Lock lock(mutex);

    if (!valid) {
        return std::nullopt;
    }

    return static_cast<float>(status.value.chamber_temp) / 10.0f;
}

CommunicationStatus XBuddyExtension::refresh_input(uint32_t max_age) {
    // Already locked by caller.

    const auto result = bus.read(unit, status, max_age);

    switch (result) {
    case CommunicationStatus::OK:
        valid = true;
        break;
    case CommunicationStatus::ERROR:
        valid = false;
        break;
    default:
        // SKIPPED doesn't change the validity.
        break;
    }

    return result;
}

CommunicationStatus XBuddyExtension::refresh_holding() {
    // Already locked by caller

    return bus.write(unit, requirement);
}

CommunicationStatus XBuddyExtension::refresh() {
    Lock lock(mutex);

    const auto input = refresh_input(250);
    const auto holding = refresh_holding();

    if (input == CommunicationStatus::ERROR || holding == CommunicationStatus::ERROR) {
        return CommunicationStatus::ERROR;
    } else if (input == CommunicationStatus::SKIPPED && holding == CommunicationStatus::SKIPPED) {
        return CommunicationStatus::SKIPPED;
    } else {
        return CommunicationStatus::OK;
    }
}

CommunicationStatus XBuddyExtension::initial_scan() {
    Lock lock(mutex);

    const auto input = refresh_input(0);
    requirement.dirty = true;
    return input;
}

CommunicationStatus XBuddyExtension::ping() {
    Lock lock(mutex);

    return refresh_input(0);
}

XBuddyExtension xbuddy_extension(puppyModbus, PuppyBootstrap::get_modbus_address_for_dock(Dock::XBUDDY_EXTENSION));

} // namespace buddy::puppies
