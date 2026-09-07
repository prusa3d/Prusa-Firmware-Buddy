#pragma once

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>

#include <common/circular_buffer.hpp>
#include <fifo_coder/fifo_coder.hpp>
#include <bsod/bsod.h>

namespace dwarf::loadcell {

using fifo_coder::LoadcellRecord;

/// Deferred-decision deglitch filter for the loadcell sample stream.
///
/// Holds suspect (out-of-threshold) samples until it can decide retroactively:
/// if the deviation reverts within MAX_SKIPPED samples it was a glitch → drop;
/// if it persists past MAX_SKIPPED it is a real signal → flush all held samples
/// in order with their original values and timestamps, leaving no gap.
///
/// Single-owner invariant: all state is touched only from the modbus task
/// (get_loadcell_sample / loadcell_set_enable). No locking needed here.
class LoadcellDeglitcher {
public:
    static constexpr size_t RUNNING_AVERAGE_STEPS = 10;
    static constexpr int32_t MAX_DIFFERENCE = 50000;
    static constexpr size_t MAX_SKIPPED = 3;

    /// Sentinel for read-timeout / undefined HX717 value.
    static constexpr int32_t undefined_value = std::numeric_limits<int32_t>::min();

    /// Pull the next output sample.
    ///
    /// Pulls raw input via try_get_input (bool(LoadcellRecord&)).
    /// Returns std::nullopt when no output is ready (input exhausted and no
    /// burst to flush); undecided suspects stay held for the next call.
    template <typename TryGet>
    std::optional<LoadcellRecord> next(TryGet &&try_get_input) {
        LoadcellRecord out;
        if (flushing) {
            [[maybe_unused]] const bool get_ok = held.try_get(out);
            debug_assert(get_ok);
            if (held.size() == 0) {
                flushing = false;
            }
            return out;
        }

        for (;;) {
            if (!try_get_input(out)) {
                return std::nullopt;
            }

            const int32_t raw = static_cast<int32_t>(out.loadcell_raw_value);
            // Compare against the average before this sample contributes to it.
            const bool within = is_within_threshold(raw);
            update_average(raw);

            if (within) {
                // Good sample: drop any held glitch candidates and emit this one.
                held.clear();
                return out;
            }

            // Suspect: stash it.
            [[maybe_unused]] const bool put_ok = held.try_put(out);
            debug_assert(put_ok);

            if (held.size() > MAX_SKIPPED) {
                // Deviation persisted — it is a real signal; flush all held samples.
                flushing = true;
                [[maybe_unused]] const bool get_ok = held.try_get(out);
                debug_assert(get_ok);
                return out;
            }
        }
    }

    /// Discard any held suspects. The running average is intentionally kept
    /// across enable/disable cycles.
    void discard_pending() {
        held.clear();
        flushing = false;
    }

private:
    bool is_within_threshold(int32_t raw) const {
        if (raw == undefined_value) {
            return false;
        }
        if (!average_valid) {
            return true; // first valid sample seeds the baseline
        }
        return std::llabs(static_cast<int64_t>(raw) - static_cast<int64_t>(running_average)) < MAX_DIFFERENCE;
    }

    void update_average(int32_t raw) {
        if (raw == undefined_value) {
            return;
        }
        if (!average_valid) {
            // Seed from the first valid sample to avoid a long warm-up from zero.
            running_average = raw;
            average_valid = true;
            return;
        }
        running_average = static_cast<int32_t>(
            (RUNNING_AVERAGE_STEPS * static_cast<int64_t>(running_average) + raw) / (RUNNING_AVERAGE_STEPS + 1));
    }

    // Room for MAX_SKIPPED suspects plus the sample that confirms a real edge.
    // CircularBuffer usable capacity is N-1 and N must be a power of two.
    static constexpr size_t held_buffer_size = 8;
    static_assert(held_buffer_size - 1 >= MAX_SKIPPED + 1);

    int32_t running_average = 0;
    bool average_valid = false;
    CircularBuffer<LoadcellRecord, held_buffer_size> held {};
    bool flushing = false;
};

} // namespace dwarf::loadcell
