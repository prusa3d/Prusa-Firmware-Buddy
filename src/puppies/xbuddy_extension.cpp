#include <puppies/xbuddy_extension.hpp>
#include <puppies/PuppyBootstrap.hpp>
#include <cassert>
#include "timing.h"

#include <logging/log.hpp>
LOG_COMPONENT_REF(MMU2);

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

uint8_t XBuddyExtension::get_requested_fan_pwm(size_t fan_idx) {
    Lock lock(mutex);

    assert(fan_idx < FAN_CNT);

    return requirement.value.fan_pwm[fan_idx];
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

void XBuddyExtension::set_usb_power(bool enabled) {
    Lock lock(mutex);

    if (enabled != requirement.value.usb_power_enable) {
        requirement.value.usb_power_enable = enabled;
        requirement.dirty = true;
    }
}

void XBuddyExtension::set_mmu_power(bool enabled) {
    Lock lock(mutex);

    if (enabled != requirement.value.mmu_power_enable) {
        requirement.value.mmu_power_enable = enabled;
        requirement.dirty = true;
    }
}

void XBuddyExtension::set_mmu_nreset(bool enabled) {
    Lock lock(mutex);

    if (enabled != requirement.value.mmu_nreset) {
        requirement.value.mmu_nreset = enabled;
        requirement.dirty = true;
    }
}

std::optional<uint16_t> XBuddyExtension::get_fan_rpm(size_t fan_idx) {
    Lock lock(mutex);

    assert(fan_idx < FAN_CNT);

    if (!valid) {
        return std::nullopt;
    }

    return static_cast<uint16_t>(status.value.fan_rpm[fan_idx]);
}

std::optional<float> XBuddyExtension::get_chamber_temp() {
    Lock lock(mutex);

    if (!valid) {
        return std::nullopt;
    }

    return static_cast<float>(status.value.chamber_temp) / 10.0f;
}

std::optional<XBuddyExtension::FilamentSensorState> XBuddyExtension::get_filament_sensor_state() {
    Lock lock(mutex);

    if (!valid) {
        return std::nullopt;
    }

    return static_cast<FilamentSensorState>(status.value.filament_sensor_state);
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
    log_info(MMU2, "post_read_mmu_register %" PRIu8, modbus_address);
    mmuValidResponseReceived = false;
    mmuModbusRq = MMUModbusRequest::make_read_register(modbus_address);
}

void XBuddyExtension::post_write_mmu_register(const uint8_t modbus_address, const uint16_t value) {
    std::lock_guard lock(mutex);
    log_info(MMU2, "post_write_mmu_register %" PRIu8, modbus_address);
    mmuValidResponseReceived = false;
    mmuModbusRq = MMUModbusRequest::make_write_register(modbus_address, value);
}

void XBuddyExtension::post_query_mmu() {
    std::lock_guard lock(mutex);
    log_info(MMU2, "post_query_mmu");
    mmuValidResponseReceived = false;
    mmuModbusRq = MMUModbusRequest::make_query();
}

void XBuddyExtension::post_command_mmu(uint8_t command, uint8_t param) {
    std::lock_guard lock(mutex);
    log_info(MMU2, "post_command_mmu");
    mmuValidResponseReceived = false;
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
    request.u.command.cp = puppy::xbuddy_extension::mmu::pack_command(command, param);
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
        auto rv = bus.read_holding(puppy::xbuddy_extension::mmu::modbusUnitNr, &mmuModbusRq.u.read.value, 1, mmuModbusRq.u.read.address, mmuModbusRq.timestamp_ms, 0);
        log_info(MMU2, "read holding(uni=%" PRIu8 " val=%" PRIu16 " adr=%" PRIu16 " ts=%" PRIu32 " rv=%" PRIu8,
            puppy::xbuddy_extension::mmu::modbusUnitNr, mmuModbusRq.u.read.value, mmuModbusRq.u.read.address, mmuModbusRq.timestamp_ms, (uint8_t)rv);
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
        auto rv = bus.write_holding(puppy::xbuddy_extension::mmu::modbusUnitNr, &mmuModbusRq.u.write.value, 1, mmuModbusRq.u.write.address, dirty);
        mmuModbusRq.timestamp_ms = last_ticks_ms(); // write_holding doesn't update the timestamp, must be done manually
        log_info(MMU2, "write holding(uni=%" PRIu8 " val=%" PRIu16 " adr=%" PRIu16 " ts=%" PRIu32 " rv=%" PRIu8,
            puppy::xbuddy_extension::mmu::modbusUnitNr, mmuModbusRq.u.write.value, mmuModbusRq.u.write.address, mmuModbusRq.timestamp_ms, (uint8_t)rv);
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
        auto rv = bus.read(puppy::xbuddy_extension::mmu::modbusUnitNr, mmuQuery, 0);
        log_info(MMU2, "read=%" PRIu8, (uint8_t)rv);
        if (rv == CommunicationStatus::OK) {
            mmuModbusRq.timestamp_ms = mmuQuery.last_read_timestamp_ms;
            mmuValidResponseReceived = true;
        } // otherwise ignore the timestamp, MMU state machinery will time out on its own
        return rv;
    }

    case MMUModbusRequest::RW::command: {
        mmuModbusRq.rw = MMUModbusRequest::RW::command_inactive; // deactivate as it will be processed shortly
        bool dirty = true; // force send the MODBUS message
        log_info(MMU2, "command");
        auto rv = bus.write_holding(puppy::xbuddy_extension::mmu::modbusUnitNr, &mmuModbusRq.u.command.cp, 1, puppy::xbuddy_extension::mmu::commandInProgressRegisterAddress, dirty);
        if (rv != CommunicationStatus::OK) {
            // log_info(MMU2, "command failed");
            return rv;
        }

        // log_info(MMU2, "command query result");

        // This is an ugly hack:
        // - First push sent command into local registers' copy - the MMU would respond with it anyway and protcol_logic expects it to be there.
        // - Then read back just the command status (register 254).
        //   Do not issue the Query directly (which would happen by querying register set 253-255)
        //   as it would send a Q0 into the MMU which would replace the command accepted/rejected in register 254.
        //
        // Beware: this command's round-trip may span over 10-20 ms which is close to the MODBUS timeout which is being used for the MMU protocol_logic as well.
        // If the round-trips become longer, MMU protocol_logic must get a larger timeout in mmu_response_received (should cause no harm afterall)
        mmuQuery.value.cip = mmuModbusRq.u.command.cp;
        rv = bus.read_holding(puppy::xbuddy_extension::mmu::modbusUnitNr, &mmuQuery.value.commandStatus, 1, puppy::xbuddy_extension::mmu::commandStatusRegisterAddress, mmuModbusRq.timestamp_ms, 0);

        if (rv == CommunicationStatus::OK) {
            // log_info(MMU2, "command query result ok");
            // timestamp_ms has been updated by bus.read_holding
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

    if (td >= 0) {
        log_info(MMU2, "mmu_response_received mmr.ts=%" PRIu32 " rqst=%" PRIu32 " td=%" PRIi32, mmuModbusRq.timestamp_ms, rqSentTimestamp_ms, td);
    } // else drop negative time differences - avoid flooding the syslog
    return mmuValidResponseReceived && td >= 0 && td < PuppyModbus::MODBUS_READ_TIMEOUT_MS;
}

#ifndef UNITTESTS
XBuddyExtension xbuddy_extension(puppyModbus, PuppyBootstrap::get_modbus_address_for_dock(Dock::XBUDDY_EXTENSION));
#else
XBuddyExtension xbuddy_extension(puppyModbus, 42);
#endif

} // namespace buddy::puppies
