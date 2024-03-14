/// @file finda.h
#pragma once
#include <stdint.h>
#include "debouncer.h"
#include "../config/config.h"

namespace modules {

/// The finda namespace provides all necessary facilities related to the logical model of the FINDA device the MMU unit.
namespace finda {

/// A model of the FINDA - basically acts as a button with pre-set debouncing
class FINDA : protected debounce::Debouncer {
public:
    inline constexpr FINDA()
        : debounce::Debouncer(config::findaDebounceMs) {};

    /// Performs one step of the state machine - reads the digital pin, processes debouncing, updates states of FINDA
    void Step();

    /// Initialize the FINDA - waits at least config::findaDebounceMs
    /// to set correct FINDA state at startup
    void BlockingInit();

    using debounce::Debouncer::Pressed;
};

/// The one and only instance of FINDA in the FW
extern FINDA finda;

} // namespace finda
} // namespace modules

namespace mf = modules::finda;
