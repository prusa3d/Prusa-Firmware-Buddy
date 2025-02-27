#include "standard_ramming_sequence.hpp"

#include <config_store/store_definition.hpp>
#include <buddy/unreachable.hpp>

using namespace buddy;

const RammingSequence &buddy::standard_ramming_sequence(StandardRammingSequence seq, uint8_t hotend) {
    [[maybe_unused]] const bool is_high_flow_nozzle = config_store().nozzle_is_high_flow.get()[hotend];

    switch (seq) {

#if HAS_AUTO_RETRACT()
    case StandardRammingSequence::auto_retract: {
        // TODO: Diffrent ramming for HF and non-HF
        static_assert(PRINTER_IS_PRUSA_COREONE());
        static constexpr RammingSequenceArray seq({
            { 8, 995 },
            { -43, 6000 },
            { -8, 3000 },
            { -4, 1800 },
            { 20, 600 },
            { -20, 470 },
            { 55, 1740 },
            { -55, 6000 },
            { 20, 340 },
            { -20, 210 },
        });
        return seq;
    }
#endif
    }

    BUDDY_UNREACHABLE();
}
