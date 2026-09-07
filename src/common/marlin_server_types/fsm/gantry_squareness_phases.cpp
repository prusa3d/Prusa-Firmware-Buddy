/// @file
#include "gantry_squareness_phases.hpp"

constinit const EnumArray<PhaseGantrySquareness, PhaseResponses, PhaseGantrySquareness::_cnt> ClientResponses::gantry_squareness_responses {
    { PhaseGantrySquareness::intro, { Response::Continue, Response::Abort } },
    { PhaseGantrySquareness::preparing, {} },
    { PhaseGantrySquareness::remove_nozzles, { Response::Continue, Response::Abort } },
    { PhaseGantrySquareness::measuring, {} },
    { PhaseGantrySquareness::result_ok, { Response::Continue } },
    { PhaseGantrySquareness::result_failed, { Response::Retry, Response::Abort } },
    { PhaseGantrySquareness::reinsert_nozzles, { Response::Continue } },
};
