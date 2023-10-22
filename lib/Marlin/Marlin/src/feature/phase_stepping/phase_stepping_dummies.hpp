#pragma once

// This header provides "no-op" implementation of some phase stepping features.
// The goal is to not pollute other code with ifdefs.

#include "bsod.h"

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct move_segment_step_generator_t;

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

inline void init_step_generator_classic(
    const move_t &,
    move_segment_step_generator_t &,
    step_generator_state_t &)
{
    bsod("Phase stepping enabled when printer doesn't support it");
}

inline void init_step_generator_input_shaping(
    const move_t &,
    move_segment_step_generator_t &,
    step_generator_state_t &)
{
    bsod("Phase stepping enabled when printer doesn't support it");
}

} // namespace phase_stepping
