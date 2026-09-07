#include "nozzle_mismatch_phases.hpp"

constinit const EnumArray<PhaseNozzleMismatch, PhaseResponses, PhaseNozzleMismatch::_cnt> ClientResponses::nozzle_mismatch_responses {
    { PhaseNozzleMismatch::prompt, { Response::Continue } },
    { PhaseNozzleMismatch::dock_selection, {} }, // Menu sends variant response, no radio buttons
    { PhaseNozzleMismatch::parking, {} },
    { PhaseNozzleMismatch::dock_not_empty, { Response::Retry } },
    { PhaseNozzleMismatch::tool_lost, { Response::Continue } },
    { PhaseNozzleMismatch::homing, {} },
    { PhaseNozzleMismatch::pickup_failed, { Response::Retry, Response::Abort } },
    { PhaseNozzleMismatch::park_failed, { Response::Retry, Response::Abort } },
    { PhaseNozzleMismatch::confirm_abort, { Response::Back, Response::Continue } },
};
