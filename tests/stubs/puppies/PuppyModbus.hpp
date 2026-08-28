#pragma once

#include <cstdint>

// This is ultralightweight version of the original header used for tests

namespace buddy::puppies {

struct RequestTiming {
    uint32_t request_end_us;
    uint32_t response_end_us;
};

} // namespace buddy::puppies
