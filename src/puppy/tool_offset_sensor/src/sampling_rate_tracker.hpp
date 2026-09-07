#pragma once

#include <cstdint>
#include <bsod/bsod.h>

// Sliding ring buffer frequency tracker.
// Records every skip-th sample timestamp, computes frequency from the span.
// Defaults: N=32 entries (128 bytes), skip=32 → ~1s window at ~1kHz.
template <uint8_t N = 32>
struct SamplingRateTracker {
    uint32_t ring[N] = {};
    uint8_t head = 0;
    uint8_t count = 0;
    uint16_t skip_counter = 0;
    uint16_t skip;

    explicit SamplingRateTracker(uint16_t skip = 32)
        : skip(skip) {
        debug_assert(skip > 0);
    }

    // Call for every sample with current timestamp in microseconds.
    // Internally records only every skip-th sample.
    void record(uint32_t now_us) {
        if (++skip_counter < skip) {
            return;
        }
        skip_counter = 0;

        ring[head] = now_us;
        head = (head + 1) % N;
        if (count < N) {
            ++count;
        }
    }

    // Returns 16.8 fixed-point Hz, or 0 if not enough data.
    uint32_t get_frequency_16_8() const {
        if (count < 2) {
            return 0;
        }

        // oldest is at head when buffer is full, otherwise at 0
        uint8_t oldest_idx = (count < N) ? 0 : head;
        uint8_t newest_idx = (head == 0) ? (N - 1) : (head - 1);

        uint32_t span_us = ring[newest_idx] - ring[oldest_idx];
        if (span_us == 0) {
            return 0;
        }

        // (count - 1) intervals recorded, each representing skip actual samples
        // frequency = (count - 1) * skip / span_seconds
        //           = (count - 1) * skip * 1'000'000 / span_us
        // In 16.8 fixed-point: multiply by 256
        uint64_t numerator = uint64_t(count - 1) * skip * 256'000'000ULL;
        return static_cast<uint32_t>(numerator / span_us);
    }

    void reset() {
        head = 0;
        count = 0;
        skip_counter = 0;
    }
};
