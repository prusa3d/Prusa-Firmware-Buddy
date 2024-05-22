// TODO: This is just copy of MK4 selftest
#pragma once

#include "i_selftest.hpp"
#include "i_selftest_part.hpp"
#include "selftest_part.hpp"
#include "selftest_result_type.hpp"
#include <selftest_types.hpp>

typedef enum {
    stsIdle,
    stsStart,
    stsSelftestStart,
    stsFans,
    stsWait_fans,
    stsEnsureZAway,
    stsXAxis,
    stsYAxis,
    stsZcalib,
    stsDocks,
    stsPhaseStepping,
    stsLoadcell,
    stsWait_loadcell,
    stsNozzleDiameter,
    stsToolOffsets,
    stsZAxis, // could not be first, printer can't home at front edges without steelsheet on
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
    stmFans = to_one_hot(stsFans),
    stmWait_fans = to_one_hot(stsWait_fans),
    stmLoadcell = to_one_hot(stsLoadcell),
    stmWait_loadcell = to_one_hot(stsWait_loadcell),
    stmNozzleDiameter = to_one_hot(stsNozzleDiameter),
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
    stmDocks = to_one_hot(stsDocks),
    stmToolOffsets = to_one_hot(stsToolOffsets),
    stmPhaseStepping = to_one_hot(stsPhaseStepping),
};

// class representing whole self-test
class CSelftest : public ISelftest {
public:
    CSelftest();

public:
    virtual bool IsInProgress() const override;
    virtual bool IsAborted() const override;
    virtual bool Start(const uint64_t test_mask, const selftest::TestData test_data) override; // parent has no clue about SelftestMask_t
    virtual void Loop() override;
    virtual bool Abort() override;

protected:
    void phaseSelftestStart();
    void restoreAfterSelftest();
    virtual void next() override;
    void phaseShowResult();
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    ToolMask tool_mask = ToolMask::AllTools;
    std::array<selftest::IPartHandler *, HOTENDS> pFans;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    std::array<selftest::IPartHandler *, HOTENDS> pNozzles;
    selftest::IPartHandler *pBed;
    std::array<selftest::IPartHandler *, HOTENDS> m_pLoadcell;
    std::array<selftest::IPartHandler *, HOTENDS> pDocks;
    selftest::IPartHandler *pToolOffsets;
    std::array<selftest::IPartHandler *, HOTENDS> pFSensor;
    selftest::IPartHandler *pNozzleDiameter;
    selftest::IPartHandler *pPhaseStepping;

    SelftestResult m_result;
};
