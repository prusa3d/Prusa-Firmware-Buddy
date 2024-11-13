/**
 * @file printer_selftest.hpp
 * @author Radek Vana
 * @brief MINI selftest header in special MINI directory
 * @date 2021-09-30
 */
#pragma once

typedef enum {
    stsIdle,
    stsStart,
    stsSelftestStart,
    stsFans,
    stsWait_fans,
    stsXAxis,
    stsYAxis,
    stsZAxis, // could not be first, printer can't home at front edges without steelsheet on
    stsMoveZup,
    stsWait_axes,
    stsHeaters_noz_ena,
    stsHeaters_bed_ena,
    stsHeaters,
    stsWait_heaters,
    stsReviseSetupAfterHeaters,
    stsNet_status,
    stsSelftestStop,
    stsDidSelftestPass,
    stsFirstLayer,
    stsShow_result,
    stsResult_wait_user,
    stsFinish,
    stsFinished,
    stsAborted,
} SelftestState_t;

consteval uint32_t to_one_hot(SelftestState_t state) {
    return static_cast<uint32_t>(1) << state;
}

enum SelftestMask_t : uint32_t {
    stmNone = 0,
    stmFans = to_one_hot(stsFans),
    stmWait_fans = to_one_hot(stsWait_fans),
    stmXAxis = to_one_hot(stsXAxis),
    stmYAxis = to_one_hot(stsYAxis),
    stmZAxis = to_one_hot(stsZAxis),
    stmMoveZup = to_one_hot(stsMoveZup),
    stmXYAxis = stmXAxis | stmYAxis,
    stmXYZAxis = stmXAxis | stmYAxis | stmZAxis,
    stmWait_axes = to_one_hot(stsWait_axes),
    stmHeaters_noz = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_noz_ena) | to_one_hot(stsReviseSetupAfterHeaters),
    stmHeaters_bed = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_bed_ena) | to_one_hot(stsReviseSetupAfterHeaters),
    stmHeaters = stmHeaters_bed | stmHeaters_noz,
    stmWait_heaters = to_one_hot(stsWait_heaters),
    stmSelftestStart = to_one_hot(stsSelftestStart),
    stmSelftestStop = to_one_hot(stsSelftestStop),
    stmNet_status = to_one_hot(stsNet_status),
    stmShow_result = to_one_hot(stsShow_result) | to_one_hot(stsResult_wait_user),
    stmFullSelftest = stmFans | stmXYZAxis | stmHeaters | stmNet_status | stmShow_result | to_one_hot(stsDidSelftestPass),
    stmFirstLayer = to_one_hot(stsFirstLayer),
};
