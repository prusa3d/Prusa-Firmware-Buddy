#pragma once

#include <option/has_auto_retract.h>

#include "ramming_sequence.hpp"

namespace buddy {

enum class StandardRammingSequence {
#if HAS_AUTO_RETRACT()
    auto_retract,
#endif

    // TODO: Migrate other ramming sequences from the Configuration files
};

const RammingSequence &standard_ramming_sequence(StandardRammingSequence seq, uint8_t hotend);

} // namespace buddy
