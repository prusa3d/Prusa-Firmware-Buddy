#pragma once

#include <cstdint>

namespace MMU2 {

/// A leaky bucket algorithm for checking if we should ask the user to do an extruder maintanence.
///
/// This keeps track if there are too many failures "recently", compared to the number of tool changes.
///
/// https://en.wikipedia.org/wiki/Leaky_bucket
class FailLeakyBucket {
public:
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
