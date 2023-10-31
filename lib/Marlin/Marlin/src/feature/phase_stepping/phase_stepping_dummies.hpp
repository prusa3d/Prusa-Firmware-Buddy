#pragma once

// This header provides "no-op" implementation of some phase stepping features.
// The goal is to not pollute other code with ifdefs.

namespace phase_stepping {

class EnsureStateDummy {
public:
    EnsureStateDummy() = default;
    ~EnsureStateDummy() {
        // This is work-around for not-triggering unused variable warning on the
        // dummy guard usage
        __asm__ __volatile__("nop;\n\t");
    };
    void release() {};
};

using EnsureEnabled = EnsureStateDummy;
using EnsureDisabled = EnsureStateDummy;

} // namespace phase_stepping
