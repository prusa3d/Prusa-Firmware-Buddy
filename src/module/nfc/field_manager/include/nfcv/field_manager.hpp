#pragma once

#include <nfcv/error.hpp>
#include <nfcv/rw_interface.hpp>

#include <cstdint>
#include <optional>

namespace nfcv {

/// Owns the RF field's lifetime: which antenna radiates, for whom, and until when.
///
/// Callers state intent by holding a \ref Guard; this class decides what the hardware does.
/// A guard's release drops the field unless the caller asked to keep it up, which is what makes
/// dropping the default and lingering the deliberate exception.
class FieldManager {
public:
    using AntennaID = ReaderWriterInterface::AntennaID;

    /// Millisecond clock. freertos::millis on hardware.
    using ClockSource = uint32_t (*)();

    /// Blocks the calling task. freertos::delay on hardware.
    using DelayFn = void (*)(uint32_t delay_ms);

    /// Source of randomness for the arbitration backoff. Need not be a good one, but it must
    /// differ between boards - otherwise contending boards back off in lockstep.
    /// !!! May have weak low bits - scale the value rather than taking a modulo of it.
    using RandomSource = uint32_t (*)();

    /// Everything the arbitration needs from the platform
    struct System {
        ClockSource now_ms;
        DelayFn delay_ms;
        RandomSource random_u32;
    };

    /// Randomised backoff between two collision-avoidance attempts
    static constexpr uint32_t retry_delay_min_ms = 5;
    static constexpr uint32_t retry_delay_max_ms = 50;

    /// Trailing grace period: how long the field keeps radiating after a keep_field_up()
    /// release, so a burst of operations does not arbitrate for every one of them.
    /// Re-armed by every release.
    static constexpr uint32_t linger_ms = 500;

    /// Absolute deadline: how long the field may radiate continuously, lingers included,
    /// measured from the moment it went up. Releases do not re-arm it, so a steady drip of
    /// operations cannot hold the field forever - the first release past the deadline drops
    /// it and hands contending neighbours a window.
    static constexpr uint32_t hold_max_ms = 1500;

    /// Whether an acquisition may inherit a field that is already radiating
    enum class Freshness : uint8_t {
        /// The linger's whole point: no power cycle if the field is already up
        reuse,

        /// Drop first, so tags power-cycle. A quieted tag answers Inventory only after the
        /// field genuinely went away.
        fresh,
    };

    /// Holds the field for as long as it lives.
    /// Movable so that callers can hand one out of a factory function; a moved-from guard is
    /// disengaged and releases nothing.
    class Guard {
    public:
        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;
        Guard &operator=(Guard &&) = delete;

        Guard(Guard &&other)
            : manager_ { other.manager_ }
            , result_ { other.result_ }
            , antenna_ { other.antenna_ }
            , retry_timeout_ms_ { other.retry_timeout_ms_ }
            , keep_up_ { other.keep_up_ } {
            other.manager_ = nullptr;
        }

        ~Guard() {
            if (manager_ && result_.has_value()) {
                manager_->release(keep_up_);
            }
        }

        explicit operator bool() const {
            return result_.has_value();
        }

        [[nodiscard]] Error error() const {
            return result_.error();
        }

        /// Keep radiating for \ref linger_ms after release, for an operation that expects a
        /// follow-up. Subject to \ref hold_max_ms.
        void keep_field_up() {
            keep_up_ = true;
        }

        /// Drops the field and raises it again on the same antenna, without releasing it.
        /// For an operation that has to power-cycle the tag mid-flight - a SLIX2 stops answering
        /// after a command with a wrong password.
        /// On failure nothing radiates and this guard's release becomes a no-op.
        [[nodiscard]] Result<void> power_cycle();

    private:
        friend class FieldManager;

        Guard(FieldManager &manager, Result<void> result, AntennaID antenna, uint32_t retry_timeout_ms)
            : manager_ { &manager }
            , result_ { result }
            , antenna_ { antenna }
            , retry_timeout_ms_ { retry_timeout_ms } {}

        FieldManager *manager_;
        Result<void> result_;
        AntennaID antenna_;
        uint32_t retry_timeout_ms_;
        bool keep_up_ = false;
    };

    FieldManager(ReaderWriterInterface &reader, const System &system)
        : reader_ { reader }
        , system_ { system } {}

    /// Raises the field on \p antenna and hands out the guard that owns it.
    /// A contended field is re-arbitrated with randomised backoff for up to \p retry_timeout_ms,
    /// blocking the calling task; one attempt is always made, so 0 means a single try. How long
    /// to wait is the caller's policy, since a background scan and a host-requested operation are
    /// worth very different waits
    [[nodiscard]] Guard acquire(AntennaID antenna, uint32_t retry_timeout_ms, Freshness freshness);

    /// Expires the linger. Driven from the owning task's loop, so the field drops on time even
    /// when nothing else touches the reader.
    void tick();

    /// Drops the field and forgets all bookkeeping
    void drop_field();

    /// Whether the field is radiating - held by a guard or lingering
    [[nodiscard]] bool is_field_up() const {
        return hold_.has_value();
    }

private:
    /// Raises the field and takes over the hold bookkeeping. The one place that owns the
    /// "the field went up from down, so the cap counts from here" rule.
    [[nodiscard]] Result<void> raise(AntennaID antenna, uint32_t retry_timeout_ms);

    /// Re-runs the reader's single-attempt bring-up with randomised backoff for as long as it
    /// reports field_collision, up to \p retry_timeout_ms. Always makes at least one attempt.
    [[nodiscard]] Result<void> arbitrate(AntennaID antenna, uint32_t retry_timeout_ms);

    /// Ends the hold a guard had: lingers if it asked to, drops the field otherwise
    void release(bool keep_up);

    struct Hold {
        /// The antenna that radiates
        AntennaID antenna;

        /// When the field went up from down, an antenna switch included.
        /// A burst bridged by a linger reuses the hold and keeps its stamp, so hold_max_ms
        /// measures the whole hold rather than its last operation.
        uint32_t since_ms;
    };

    ReaderWriterInterface &reader_;
    const System system_;

    /// The field we currently hold. Set exactly while the field radiates, which is what
    /// \ref is_field_up reports.
    std::optional<Hold> hold_;

    /// When the last guard released with keep_field_up(). Set only while lingering.
    std::optional<uint32_t> idle_since_ms_;
};

} // namespace nfcv
