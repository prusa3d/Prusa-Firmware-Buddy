#include <nfcv/field_manager.hpp>

#include <logging/log.hpp>

// Set this to info to get field up/down messages to debug field acquisition
LOG_COMPONENT_DEF(NfcField, logging::Severity::warning);

namespace nfcv {

namespace {

    /// Maps \p random onto [lo, hi], both ends inclusive.
    /// Scales instead of taking a modulo - the platform's RNG may have poor low bits.
    constexpr uint32_t scale_to_range(uint32_t random, uint32_t lo, uint32_t hi) {
        return lo + static_cast<uint32_t>((static_cast<uint64_t>(random) * (hi - lo + 1)) >> 32);
    }
} // namespace

FieldManager::Guard FieldManager::acquire(AntennaID antenna, uint32_t retry_timeout_ms, Freshness freshness) {
    if (freshness == Freshness::fresh) {
        // Unconditional: the reader early-outs on an already dead field, and this keeps the tag
        // power cycle independent of our own bookkeeping
        drop_field();
    }

    return Guard { *this, raise(antenna, retry_timeout_ms), antenna, retry_timeout_ms };
}

Result<void> FieldManager::raise(AntennaID antenna, uint32_t retry_timeout_ms) {
    // Already radiating where the caller wants it, and re-raising would power-cycle the tags
    // mid-burst. Callers that want a power cycle drop the field first.
    if (hold_.has_value() && hold_->antenna == antenna) {
        idle_since_ms_.reset();
        return {};
    }

    if (const auto res = arbitrate(antenna, retry_timeout_ms); !res.has_value()) {
        // try_field_up leaves nothing radiating when it fails
        hold_.reset();
        idle_since_ms_.reset();
        return res;
    }

    log_info(NfcField, "field up on antenna %u", static_cast<unsigned>(antenna));

    hold_ = Hold {
        .antenna = antenna,
        .since_ms = system_.now_ms(),
    };
    idle_since_ms_.reset();

    return {};
}

Result<void> FieldManager::arbitrate(AntennaID antenna, uint32_t retry_timeout_ms) {
    const uint32_t start_ms = system_.now_ms();

    for (;;) {
        const auto res = reader_.try_field_up(antenna);

        // Unsigned difference - the clock wraps
        const uint32_t elapsed_ms = system_.now_ms() - start_ms;

        if (!res.has_value() && res.error() == Error::field_collision && elapsed_ms < retry_timeout_ms) {
            system_.delay_ms(scale_to_range(system_.random_u32(), retry_delay_min_ms, retry_delay_max_ms));
            log_info(NfcField, "Field up on antenna %u congestion detected, waiting", static_cast<unsigned>(antenna));
            continue;
        }

        if (!res.has_value()) {
            log_warning(NfcField, "field up on antenna %u failed after %u ms: %s", static_cast<unsigned>(antenna), static_cast<unsigned>(elapsed_ms), to_str(res.error()));
        }

        return res;
    }
}

void FieldManager::release(bool keep_up) {
    if (!hold_.has_value()) {
        // Something dropped the field under us - nothing to release, and arming a linger over a
        // dead field would fire a drop at nothing
        return;
    }

    const uint32_t now = system_.now_ms();

    // Unsigned difference - the clock wraps
    if (!keep_up || static_cast<uint32_t>(now - hold_->since_ms) >= hold_max_ms) {
        drop_field();
        return;
    }

    idle_since_ms_ = now;
}

void FieldManager::tick() {
    if (!idle_since_ms_.has_value()) {
        return;
    }

    if (static_cast<uint32_t>(system_.now_ms() - *idle_since_ms_) >= linger_ms) {
        drop_field();
    }
}

void FieldManager::drop_field() {
    if (hold_.has_value()) {
        log_info(NfcField, "field down on antenna %u", static_cast<unsigned>(hold_->antenna));
    }

    reader_.field_down();
    hold_.reset();
    idle_since_ms_.reset();
}

Result<void> FieldManager::Guard::power_cycle() {
    // Release-then-raise is the whole point: the tag only resets if the field genuinely went
    // away. raise() restamps the hold, since the field restarted here.
    manager_->drop_field();
    return manager_->raise(antenna_, retry_timeout_ms_);
}

} // namespace nfcv
