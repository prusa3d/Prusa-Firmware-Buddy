/**
 * @file
 * @author Radek Vana
 * @brief iX selftest header in special iX directory
 * @date 2021-09-30
 */
#pragma once

#include <option/has_mmu2.h>

typedef enum {
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
    stmHeaters_noz = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_noz_ena),
    stmHeaters_bed = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_bed_ena),
    stmHeaters = stmHeaters_bed | stmHeaters_noz,
    stmWait_heaters = to_one_hot(stsWait_heaters),
    stmFSensor = to_one_hot(stsFSensor_calibration),
    stmSelftestStart = to_one_hot(stsSelftestStart),
    stmSelftestStop = to_one_hot(stsSelftestStop),
};
