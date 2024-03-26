#include "fail_bucket.hpp"

#include <config_store/store_instance.hpp>
#include <limits>

namespace {

// We remove one failure every 1000 successful loads.
constexpr uint32_t leak_every = 1000;
// If we accumulate 50 failures (either because they happen all at once or
// because they slowly accumulate over time), the bucket overflows.
constexpr uint16_t overflow_limit = 50;

} // namespace

namespace MMU2 {

FailLeakyBucket FailLeakyBucket::instance;

void FailLeakyBucket::add_failure() {
    const auto value = config_store().mmu_fail_bucket.get();
    if (value < std::numeric_limits<decltype(value)>::max()) {
        // Don't overflow the value (unlikely anyway)
        config_store().mmu_fail_bucket.set(value + 1);
    }
}

void FailLeakyBucket::success(uint32_t successes) {
    const auto value = config_store().mmu_fail_bucket.get();
    if ((successes % leak_every == 0) && (value > 0)) {
        config_store().mmu_fail_bucket.set(value - 1);
    }
}

bool FailLeakyBucket::reached_limit() const {
    return config_store().mmu_fail_bucket.get() >= overflow_limit;
}

} // namespace MMU2
