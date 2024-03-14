#pragma once

#include <cstdint>

// This is ultralightweight version of the original header used for tests

namespace buddy::puppies {

struct RequestTiming {
    uint32_t begin_us;
    uint32_t end_us;
};

} // namespace buddy::puppies
