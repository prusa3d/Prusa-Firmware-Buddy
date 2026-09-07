#pragma once

#include <cstdint>
#include <limits>
#include <bsod/bsod.h>

/// @brief The LeakyBucket class implements the Leaky Bucket algorithm for rate limiting.
class LeakyBucket {
public:
    /// @Constructor.
    ///
    /// Parameters the same as set_parameters.
    constexpr LeakyBucket(uint32_t capacity = 1, uint32_t interval = 1) {
        set_parameters(capacity, interval);
    }
    /// @brief Sets the parameters for the leaky bucket.
    /// @param capacity Maximum number of samples that can be held in the
    ///   bucket. If set to 0, it's always "full" and no samples ever fit.
    /// @param interval How long (in units of the timestamp) does it take to
    ///   leak one sample. If set to 0, the bucket never fills and all samples
    ///   always fit.
    ///
    /// In case both are 0, the rule about interval takes precedence - it always
    /// fits.
    void set_parameters(uint32_t capacity, uint32_t interval) {
        debug_assert(static_cast<uint64_t>(capacity) * static_cast<uint64_t>(interval) < static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
        this->interval = interval;
        // We keep the size in the amount needed to clear the bucket, not in samples.
        this->capacity = capacity * interval;
    }

    /// @brief Resets the leaky bucket to an empty state.
    void reset() {
        current_size = 0;
        // It's OK to leave timestamp at some "random" (last added sample)
        // value. It only influences how much leakes when adding the next
        // sample - and the bucket is empty, so nothing leaks.
    }

    /// @brief Adds a sample to the leaky bucket.
    /// @param timestamp The arrival time of the sample (in milliseconds)
    /// @return bool True if the sample fits in the bucket, false otherwise
    [[nodiscard]] bool add_sample(uint32_t timestamp) {
        const uint32_t leaked = timestamp - last_timestamp;
        last_timestamp = timestamp;

        if (current_size > leaked) {
            // Only part of it leaked out
            current_size -= leaked;
        } else {
            // Everything left leaked out
            current_size = 0;
        }

        // Add one sample of interval size.
        const uint32_t new_size = current_size + interval;

        if (new_size <= capacity) {
            current_size = new_size;
            return true;
        } else {
            // It either fits completely or not at all.
            return false;
        }
    }

private:
    // Size of one sample in terms of timestamp units.
    //
    // (that is, how long it takes to leak one sample)
    uint32_t interval = 1;

    // Capacity of the bucket (again measured in the time it takes to leak completely).
    uint32_t capacity = 1;

    // Current fill of the bucket (between 0 and capacity).
    uint32_t current_size = 0;

    // Last time we added a sample to the bucket.
    uint32_t last_timestamp = 0;
};
