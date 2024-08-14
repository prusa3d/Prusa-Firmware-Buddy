#pragma once

#include <cstdint>

namespace MMU2 {

/// A leaky bucket algorithm for checking if we should ask the user to do an extruder maintanence.
///
/// This keeps track if there are too many failures "recently", compared to the number of tool changes.
///
/// https://en.wikipedia.org/wiki/Leaky_bucket
class FailLeakyBucket {
private:
    // We remove one failure every X loads.
    uint32_t leak_every;
    // If we accumulate X failures (either because the happen all at once, or
    // because they slowly accumulate over time), the bucket overflows.
    uint16_t overflow_limit;

public:
    FailLeakyBucket(uint32_t leak_every = 1000, uint16_t overflow_limit = 50)
        : leak_every(leak_every)
        , overflow_limit(overflow_limit) {}
    /// Add one failure.
    void add_failure();
    /// A success happened.
    ///
    /// The successes is the _total number_ of successes, not incremental
    /// change.
    void success(uint32_t successes);
    /// Is the bucket in an overflown state?
    bool reached_limit() const;

    static FailLeakyBucket instance;
};

} // namespace MMU2
