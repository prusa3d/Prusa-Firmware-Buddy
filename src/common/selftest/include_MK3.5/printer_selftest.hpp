/**
 * @file printer_selftest.hpp
 * @author Radek Vana
 * @brief MK4 selftest header in special MK4 directory
 * @date 2021-09-30
 */
#pragma once

#include "i_selftest.hpp"
#include "selftest_part.hpp"
#include "selftest_result_type.hpp"
#include "config_features.h"

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
    stsZcalib,
    stsXAxis,
    stsYAxis,
    stsZAxis, // could not be first, printer can't home at front edges without steelsheet on
    stsMoveZup,
    stsWait_axes,
    stsHeaters_noz_ena,
    stsHeaters_bed_ena,
    stsHeaters,
    stsWait_heaters,
    stsHotendSpecify,
    stsNet_status,
    stsSelftestStop,
    stsDidSelftestPass,
    stsEpilogue_nok,
    stsEpilogue_nok_wait_user,
    stsFirstLayer,
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
    stmZcalib = to_one_hot(stsZcalib),
    stmXAxis = to_one_hot(stsXAxis),
    stmYAxis = to_one_hot(stsYAxis),
    stmZAxis = to_one_hot(stsZAxis),
    stmMoveZup = to_one_hot(stsMoveZup),
    stmXYAxis = stmXAxis | stmYAxis,
    stmXYZAxis = stmXAxis | stmYAxis | stmZAxis,
    stmWait_axes = to_one_hot(stsWait_axes),
    stmHeaters_noz = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_noz_ena) | to_one_hot(stsHotendSpecify),
    stmHeaters_bed = to_one_hot(stsHeaters) | to_one_hot(stsHeaters_bed_ena),
    stmHeaters = stmHeaters_bed | stmHeaters_noz,
    stmWait_heaters = to_one_hot(stsWait_heaters),
    stmSelftestStart = to_one_hot(stsSelftestStart),
    stmSelftestStop = to_one_hot(stsSelftestStop),
    stmNet_status = to_one_hot(stsNet_status),
    stmShow_result = to_one_hot(stsShow_result) | to_one_hot(stsResult_wait_user),
    stmFullSelftest = stmFans | stmXYZAxis | stmHeaters | stmNet_status | stmShow_result | to_one_hot(stsDidSelftestPass),
    stmWizardPrologue = to_one_hot(stsPrologueAskRun) | to_one_hot(stsPrologueAskRun_wait_user) | to_one_hot(stsPrologueInfo) | to_one_hot(stsPrologueInfo_wait_user) | to_one_hot(stsPrologueInfoDetailed) | to_one_hot(stsPrologueInfoDetailed_wait_user),
    stmEpilogue = to_one_hot(stsEpilogue_nok) | to_one_hot(stsEpilogue_nok_wait_user) | to_one_hot(stsEpilogue_ok) | to_one_hot(stsEpilogue_ok_wait_user),
    stmFirstLayer = to_one_hot(stsFirstLayer),
    stmWizard = stmFullSelftest | stmWizardPrologue | stmEpilogue | stmFirstLayer,
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
    uint8_t previous_sheet_index {};
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    std::array<selftest::IPartHandler *, HOTENDS> pFans;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    std::array<selftest::IPartHandler *, HOTENDS> pNozzles;
    selftest::IPartHandler *pBed;
    selftest::IPartHandler *pHotendSpecify;
    selftest::IPartHandler *pFirstLayer;

    SelftestResult m_result;
};
