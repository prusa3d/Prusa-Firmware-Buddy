#include "wastebin_emptying_phases.hpp"

constinit const EnumArray<PhaseWastebinEmptying, PhaseResponses, PhaseWastebinEmptying::_cnt> ClientResponses::wastebin_emptying_responses {
    { PhaseWastebinEmptying::parking, {} },
    { PhaseWastebinEmptying::prompt_full, { Response::Ignore, Response::Done } },
    { PhaseWastebinEmptying::prompt_empty, { Response::Done } },
    { PhaseWastebinEmptying::raising_bed, {} },
    { PhaseWastebinEmptying::heating, {} },
    { PhaseWastebinEmptying::purging, {} },
    { PhaseWastebinEmptying::returning, {} },
};

constinit const EnumBitset<PhaseWastebinEmptying, PhaseWastebinEmptying::_cnt> wastebin_emptying_is_phase_attention {
    { PhaseWastebinEmptying::parking, false },
    { PhaseWastebinEmptying::prompt_full, true },
    { PhaseWastebinEmptying::prompt_empty, true },
    { PhaseWastebinEmptying::raising_bed, false },
    { PhaseWastebinEmptying::heating, false },
    { PhaseWastebinEmptying::purging, false },
    { PhaseWastebinEmptying::returning, false },
};
