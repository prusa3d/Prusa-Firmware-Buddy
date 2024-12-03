// TODO: This is just copy of MK4 selftest
#pragma once

#include <selftest_types.hpp>

typedef enum {
    stsIdle,
    stsStart,
    stsSelftestStart,
    stsEnsureZAway,
    stsXAxis,
    stsYAxis,
    stsZcalib,
    stsDocks,
    stsLoadcell,
    stsWait_loadcell,
    stsToolOffsets,
    stsZAxis, // could not be first, printer can't home at front edges without steelsheet on
    stsWait_axes,
    stsHeaters_noz_ena,
    stsHeaters_bed_ena,
    stsHeaters,
    stsWait_heaters,
    stsReviseSetupAfterHeaters,
    stsFSensor_calibration,
    stsSelftestStop,
    stsFinish,
    stsFinished,
    stsAborted,
} SelftestState_t;

consteval uint32_t to_one_hot(SelftestState_t state) {
    return static_cast<uint32_t>(1) << state;
}

enum SelftestMask_t : uint32_t {
    stmNone = 0,
    stmLoadcell = to_one_hot(stsLoadcell),
    stmWait_loadcell = to_one_hot(stsWait_loadcell),
    stmZcalib = to_one_hot(stsZcalib),
    stmEnsureZAway = to_one_hot(stsEnsureZAway),
    stmXAxis = to_one_hot(stsXAxis),
    stmYAxis = to_one_hot(stsYAxis),
    stmZAxis = to_one_hot(stsZAxis),
    stmWait_axes = to_one_hot(stsWait_axes),
    stmHeaters_noz = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_noz_ena) | to_one_hot(stsReviseSetupAfterHeaters),
    stmHeaters_bed = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_bed_ena) | to_one_hot(stsReviseSetupAfterHeaters),
    stmHeaters = stmHeaters_bed | stmHeaters_noz,
    stmWait_heaters = to_one_hot(stsWait_heaters),
    stmFSensor = to_one_hot(stsFSensor_calibration),
    stmSelftestStart = to_one_hot(stsSelftestStart),
    stmSelftestStop = to_one_hot(stsSelftestStop),
    stmDocks = to_one_hot(stsDocks),
    stmToolOffsets = to_one_hot(stsToolOffsets),
};
