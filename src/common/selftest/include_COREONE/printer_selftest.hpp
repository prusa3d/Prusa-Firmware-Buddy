/// @file
#pragma once

enum SelftestState_t : uint8_t {
    stsIdle,
    stsStart,
    stsSelftestStart,
    stsEnsureZAway,
    stsYAxis,
    stsXAxis,
    stsZcalib,
    stsLoadcell,
    stsWait_loadcell,
    stsZAxis,
    stsWait_axes,
    stsHeaters_noz_ena,
    stsHeaters_bed_ena,
    stsHeaters,
    stsWait_heaters,
    stsGears,
    stsFSensor_calibration,
    stsFSensor_flip_mmu_at_the_end,
    stsSelftestStop,
    stsFinish,
    stsFinished,
    stsAborted,
};

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
    stmHeaters_noz = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_noz_ena),
    stmHeaters_bed = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_bed_ena),
    stmHeaters = stmHeaters_bed | stmHeaters_noz,
    stmWait_heaters = to_one_hot(stsWait_heaters),
    stmFSensor = to_one_hot(stsFSensor_calibration),
    stmFSensor_flip_mmu_at_the_end = to_one_hot(stsFSensor_calibration) | to_one_hot(stsFSensor_flip_mmu_at_the_end),
    stmGears = to_one_hot(stsGears),
    stmSelftestStart = to_one_hot(stsSelftestStart),
    stmSelftestStop = to_one_hot(stsSelftestStop),
};
