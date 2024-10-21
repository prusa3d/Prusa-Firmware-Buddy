#include <puppies/xbuddy_extension.hpp>
#include <puppies/PuppyBootstrap.hpp>
#include <cassert>
#include "timing.h"


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

    // Actually, failed MMU communication is not an issue on this level. Timeout and retries are being handled on the protocol_logic level
    // while MODBUS purely serves as a pass-through transport media -> no need to panic when MMU doesn't communicate now
    // Note: might change in the future, therefore keeping the same interface like refresh_input
    refresh_mmu();

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

void XBuddyExtension::post_read_mmu_register(const uint8_t modbus_address) {
    // Post a message to the puppy task and execute the communication handshake there.
    // Fortunately, the MMU code is flexible enough - it doesn't require immediate answers
    std::lock_guard lock(mutex);
    mmuModbusRq = MMUModbusRequest::make_read_register(modbus_address);
}

void XBuddyExtension::post_write_mmu_register(const uint8_t modbus_address, const uint16_t value) {
    std::lock_guard lock(mutex);
    mmuModbusRq = MMUModbusRequest::make_write_register(modbus_address, value);
}

void XBuddyExtension::post_query_mmu() {
    std::lock_guard lock(mutex);
    mmuModbusRq = MMUModbusRequest::make_query();
}

void XBuddyExtension::post_command_mmu(uint8_t command, uint8_t param) {
    std::lock_guard lock(mutex);
    mmuModbusRq = MMUModbusRequest::make_command(command, param);
}

XBuddyExtension::MMUModbusRequest XBuddyExtension::MMUModbusRequest::make_read_register(uint8_t address) {
    MMUModbusRequest request;
    request.u.read.address = address;
    request.u.read.value = 0;
    request.rw = RW::read;
    return request;
}

XBuddyExtension::MMUModbusRequest XBuddyExtension::MMUModbusRequest::make_write_register(uint8_t address, uint16_t value) {
    MMUModbusRequest request;
    request.u.write.address = address;
    request.u.write.value = value;
    request.rw = RW::write;
    return request;
}

XBuddyExtension::MMUModbusRequest XBuddyExtension::MMUModbusRequest::make_query() {
    MMUModbusRequest request;
    request.u.query.pec = 0;
    request.rw = RW::query;
    return request;
}

XBuddyExtension::MMUModbusRequest XBuddyExtension::MMUModbusRequest::make_command(uint8_t command, uint8_t param) {
    MMUModbusRequest request;
    request.u.command.u.command = command;
    request.u.command.u.param = param;
    request.rw = RW::command;
    return request;
}

CommunicationStatus XBuddyExtension::refresh_mmu() {
    // process MMU request
    // the result is written back into the MMUModbusRequest structure
    // by definition of the MMU protocol, there is only one active request-response pair on the bus

    switch (mmuModbusRq.rw) {
    case MMUModbusRequest::RW::read: {
        mmuModbusRq.rw = MMUModbusRequest::RW::read_inactive; // deactivate as it will be processed shortly
        // even if the communication fails, the MMU state machine handles it, it is not required to performs repeats at the MODBUS level
        auto rv = bus.read_holding(mmuUnitNr, &mmuModbusRq.u.read.value, 1, mmuModbusRq.u.read.address, mmuModbusRq.timestamp_ms, 0);
        if (rv == CommunicationStatus::OK) {
            mmuModbusRq.u.read.accepted = true; // this is a bit speculative
            mmuValidResponseReceived = true;
        } else {
            mmuModbusRq.u.read.accepted = false;
        }
        return rv;
    }

    case MMUModbusRequest::RW::write: {
        mmuModbusRq.rw = MMUModbusRequest::RW::write_inactive; // deactivate as it will be processed shortly
        bool dirty = true; // force send the MODBUS message
        auto rv = bus.write_holding(mmuUnitNr, &mmuModbusRq.u.write.value, 1, mmuModbusRq.u.write.address, dirty);
        mmuModbusRq.timestamp_ms = last_ticks_ms(); // write_holding doesn't update the timestamp, must be done manually
        if (rv == CommunicationStatus::OK) {
            mmuModbusRq.u.write.accepted = true; // this is a bit speculative
            mmuValidResponseReceived = true;
        } else {
            mmuModbusRq.u.write.accepted = false;
        }
        return rv;
    }

    case MMUModbusRequest::RW::query: {
        mmuModbusRq.rw = MMUModbusRequest::RW::query_inactive; // deactivate as it will be processed shortly
        auto rv = bus.read(mmuUnitNr, mmuQuery, 0);
        if (rv == CommunicationStatus::OK) {
            mmuModbusRq.timestamp_ms = mmuQuery.last_read_timestamp_ms;
            mmuValidResponseReceived = true;
        } // otherwise ignore the timestamp, MMU state machinery will time out on its own
        return rv;
    }

    case MMUModbusRequest::RW::command: {
        mmuModbusRq.rw = MMUModbusRequest::RW::command_inactive; // deactivate as it will be processed shortly
        bool dirty = true; // force send the MODBUS message
        auto rv = bus.write_holding(mmuUnitNr, &mmuModbusRq.u.command.cp, 1, mmuCommandInProgressRegisterAddress, dirty);
        if (rv != CommunicationStatus::OK) {
            return rv;
        }

        // now read back command status - via the query registers
        rv = bus.read(mmuUnitNr, mmuQuery, 0);
        if (rv == CommunicationStatus::OK) {
            mmuModbusRq.timestamp_ms = mmuQuery.last_read_timestamp_ms;
            mmuValidResponseReceived = true;
        } // otherwise ignore the timestamp, MMU state machinery will time out on its own
        return rv;
    }

    default:
        return CommunicationStatus::SKIPPED;
    }
}

bool XBuddyExtension::mmu_response_received(uint32_t rqSentTimestamp_ms) const {
    int32_t td = ~0U;
    {
        std::lock_guard lock(mutex);
        td = ticks_diff(mmuModbusRq.timestamp_ms, rqSentTimestamp_ms);
    }

    return mmuValidResponseReceived && td >= 0 && td < PuppyModbus::MODBUS_READ_TIMEOUT_MS;
}

#ifndef UNITTESTS
XBuddyExtension xbuddy_extension(puppyModbus, PuppyBootstrap::get_modbus_address_for_dock(Dock::XBUDDY_EXTENSION));
#else
XBuddyExtension xbuddy_extension(puppyModbus, 42);
#endif

} // namespace buddy::puppies
