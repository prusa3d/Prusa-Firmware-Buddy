#include <puppies/xl_can.hpp>

#include "timing.h"
#include <logging/log.hpp>
#include <modbus/server_address.hpp>
#include <mutex>
#include <puppies/cyphal_flash_files.hpp>
#include <utility>
#include <puppies/cyphal_bridge.hpp>

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

namespace {
    using Lock = std::unique_lock<freertos::Mutex>;
    using FileId = xbuddy_extension::FileId;

    constexpr uint8_t unit = std::to_underlying(modbus::ServerAddress::xl_can);

    // Send an activity heartbeat roughly 3x per second; the bridge fw treats a
    // missing update for ~1 s as "master dead".
    constexpr int32_t activity_update_every_ms = 300;
} // namespace

CommunicationStatus XlCan::ping(PuppyModbus &bus) {
    Lock lock(mutex);
    return refresh_input(bus, 0);
}

CommunicationStatus XlCan::initial_scan(PuppyModbus &bus) {
    return ping(bus);
}

CommunicationStatus XlCan::refresh_input(PuppyModbus &bus, uint32_t max_age) {
    // Caller holds `mutex`. ping()/initial_scan() read with max_age 0 so the
    // flash host has published requests before the bootstrap wait runs: the
    // wait's first refresh() reads with a 250 ms max_age, which hits the cache
    // from initial_scan (a few ms earlier) and returns SKIPPED. Without this,
    // the host would still hold no valid request on that first refresh and
    // would fail the whole refresh, bouncing bootstrap into recovery.
    const auto result = bus.read(unit, status, max_age);
    switch (result) {
    case CommunicationStatus::OK:
        status_valid.store(true);
        flash.set_requests(status.value.chunk_request, status.value.digest_request);
        break;
    case CommunicationStatus::ERROR:
        status_valid.store(false);
        flash.invalidate();
        break;
    case CommunicationStatus::SKIPPED:
        break;
    }
    return result;
}

CommunicationStatus XlCan::refresh(PuppyModbus &bus) {
    Lock lock(mutex);

    // Read status and publish the sensor's firmware requests. Read on every
    // exchange while flashing (max_age 0) so we pick up the next chunk request
    // ASAP; throttle to 250 ms otherwise.
    const auto status_result = refresh_input(bus, flash.flashing() ? 0 : 250);
    if (status_result == CommunicationStatus::OK) {
        if (const bool fault = (status.value.fan_power_fault != 0); fault != last_fan_power_fault) {
            last_fan_power_fault = fault;
            if (fault) {
                log_warning(Puppies, "xl_can: fan power switch FAULT (overcurrent/overtemperature)");
            } else {
                log_info(Puppies, "xl_can: fan power switch fault cleared");
            }
        }
    }

    return aggregate_communication_status({
        status_result,
        refresh_holding(bus),
        flash.write_chunk(bus, modbus::ServerAddress::xl_can),
        flash.write_digest(bus, modbus::ServerAddress::xl_can, [&lock](xbuddy_extension::modbus::DigestRequest request, FileId file_id, xbuddy_extension::modbus::Digest &out) {
            // Release the mutex for the slow digest computation (file read + SHA256).
            lock.unlock();
            compute_digest_response(request, file_id, out);
            lock.lock();
        }),
        cyphal_bridge.refresh(bus, modbus::ServerAddress::xl_can),
    });
}

void XlCan::set_otp(const OTP_v5 &v) {
    Lock lock(mutex);
    otp = v;
}

OTP_v5 XlCan::get_otp() const {
    Lock lock(mutex);
    return otp;
}

void XlCan::set_fan_pwm(uint8_t pwm, FanSelftestMode selftest_mode) {
    switch (selftest_mode) {

    case FanSelftestMode::nop_if_selftest:
        if (fan_selftest_active.load()) {
            return;
        }
        break;

    case FanSelftestMode::set_selftest:
        fan_selftest_active.store(true);
        break;

    case FanSelftestMode::exit_selftest:
        fan_selftest_active.store(false);
        break;
    }
    fan_pwm_desired.store(pwm);
}

std::optional<uint16_t> XlCan::get_fan_rpm() const {
    Lock lock(mutex);
    if (!status_valid.load()) {
        return std::nullopt;
    }
    return status.value.fan_rpm[xbuddy_extension::modbus::XL_CAN_FAN_IDX];
}

CommunicationStatus XlCan::set_modular_bed_reset(PuppyModbus &bus, bool nreset) {
    Lock lock(mutex);
    mmu_nreset_desired.store(nreset);
    config.dirty = true;

    // Send immediately - needed for the modular bed boodstrap
    return refresh_holding(bus);
}

CommunicationStatus XlCan::refresh_holding(PuppyModbus &bus) {
    // Caller holds `mutex`.
    const auto write = [&](uint16_t &dst, const uint16_t val) {
        if (val != dst) {
            dst = val;
            config.dirty = true;
        }
    };
    write(config.value.mmu_nreset, static_cast<uint16_t>(mmu_nreset_desired.load() ? 1 : 0));
    write(config.value.fan_pwm[xbuddy_extension::modbus::XL_CAN_FAN_IDX], fan_pwm_desired.load());

    // Activity heartbeat: refresh the value every cycle so whatever write goes
    // out carries a fresh stamp, but only mark dirty periodically so we
    // piggy-back on other writes instead of a round trip every cycle. Mirrors
    // XBuddyExtension::refresh_holding.
    const uint32_t now = ticks_ms();
    config.value.activity = static_cast<uint16_t>(now);
    if (ticks_diff(now, last_activity_update) > activity_update_every_ms) {
        config.dirty = true;
    }

    const auto result = bus.write(unit, config);
    if (result == CommunicationStatus::OK) {
        last_activity_update = now;
    }
    return result;
}

uint8_t XlCan::get_flash_progress_percent() const {
    Lock lock(mutex);
    return flash.progress_percent();
}

XlCan xl_can;

} // namespace buddy::puppies
