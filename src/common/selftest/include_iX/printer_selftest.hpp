/**
 * @file
 * @author Radek Vana
 * @brief iX selftest header in special iX directory
 * @date 2021-09-30
 */
#pragma once

#include "i_selftest.hpp"
#include <option/has_mmu2.h>

typedef enum {
    stsIdle,
    stsStart,
    stsPrologueAskRun,
    stsPrologueAskRun_wait_user,
    stsSelftestStart,
    stsPrologueInfo,
    stsPrologueInfo_wait_user,
    stsPrologueInfoDetailed,
    stsPrologueInfoDetailed_wait_user,
    stsFans,
    stsWait_fans,
    stsLoadcell,
    stsWait_loadcell,
    stsYAxis,
    stsXAxis,
    stsMoveZup,
    stsWait_axes,
    stsHeaters_noz_ena,
    stsHeaters,
    stsWait_heaters,
    stsFSensor_calibration,
#if HAS_MMU2()
    stsFSensorMMU_calibration,
#endif
    stsNet_status,
    stsSelftestStop,
    stsDidSelftestPass,
    stsEpilogue_nok,
    stsEpilogue_nok_wait_user,
    stsShow_result,
    stsResult_wait_user,
    stsEpilogue_ok,
    stsEpilogue_ok_wait_user,
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
    stmYAxis = to_one_hot(stsYAxis),
    stmXAxis = to_one_hot(stsXAxis),
    stmMoveZup = to_one_hot(stsMoveZup),
    stmXYAxis = stmXAxis | stmYAxis,
    stmXYZAxis = stmXAxis | stmYAxis,
    stmWait_axes = to_one_hot(stsWait_axes),
    stmHeaters_noz = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_noz_ena),
    stmHeaters = stmHeaters_noz,
    stmWait_heaters = to_one_hot(stsWait_heaters),
    stmFSensor = to_one_hot(stsFSensor_calibration),
#if HAS_MMU2()
    stmFSensorMMU = to_one_hot(stsFSensorMMU_calibration),
#endif
    stmSelftestStart = to_one_hot(stsSelftestStart),
    stmSelftestStop = to_one_hot(stsSelftestStop),
    stmNet_status = to_one_hot(stsNet_status),
    stmShow_result = to_one_hot(stsShow_result) | to_one_hot(stsResult_wait_user),
    stmFullSelftest = stmFans | stmLoadcell | stmXYZAxis | stmHeaters | stmFSensor | stmNet_status | stmShow_result | to_one_hot(stsDidSelftestPass),
    stmWizardPrologue = to_one_hot(stsPrologueAskRun) | to_one_hot(stsPrologueAskRun_wait_user) | to_one_hot(stsPrologueInfo) | to_one_hot(stsPrologueInfo_wait_user) | to_one_hot(stsPrologueInfoDetailed) | to_one_hot(stsPrologueInfoDetailed_wait_user),
    stmEpilogue = to_one_hot(stsEpilogue_nok) | to_one_hot(stsEpilogue_nok_wait_user) | to_one_hot(stsEpilogue_ok) | to_one_hot(stsEpilogue_ok_wait_user),
    stmWizard = stmFullSelftest | stmWizardPrologue | stmEpilogue,
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
    bool phaseWaitUser(PhasesSelftest phase);
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    std::array<selftest::IPartHandler *, HOTENDS> pFans;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    std::array<selftest::IPartHandler *, HOTENDS> pNozzles;
    std::array<selftest::IPartHandler *, HOTENDS> m_pLoadcell;
    std::array<selftest::IPartHandler *, HOTENDS> pFSensor;

    SelftestResult m_result;
};
