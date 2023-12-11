#pragma once

#include "i_selftest_part.hpp"
#include "selftest_loop_result.hpp"
#include <common/fsm_base_types.hpp>
#include <cstdint>

namespace selftest {

struct SelftestNozzleDiameterResult {
    float selected_diameter = -1.0f;

    void Abort() {};
    void Fail() {};
    void Pass() {};
};

struct SelftestNozzleDiameterConfig {
    using type_evaluation = SelftestNozzleDiameterResult;
    static constexpr auto part_type = SelftestParts::NozzleDiameter;
    static constexpr auto part_name = "Nozzle Diameter Config";
};

class SelftestPartNozzleDiameter {
    IPartHandler &state_machine;
    SelftestNozzleDiameterResult &result;

public:
    explicit SelftestPartNozzleDiameter(IPartHandler &state_machine, const SelftestNozzleDiameterConfig &, SelftestNozzleDiameterResult &result);

    LoopResult statePrepare();
    LoopResult stateAskDefaultNozzleDiameter();
    LoopResult stateSaveResultToEeprom();
};

} // namespace selftest
