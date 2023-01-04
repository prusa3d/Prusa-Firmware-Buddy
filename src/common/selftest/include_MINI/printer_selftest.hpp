/**
 * @file printer_selftest.hpp
 * @author Radek Vana
 * @brief MINI selftest header in special MINI directory
 * @date 2021-09-30
 */
#pragma once

#include "i_selftest.hpp"
#include "super.hpp"
#include "selftest_part.hpp"
#include "selftest_result_type.hpp"

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
    stsXAxis,
    stsYAxis,
    stsZAxis, // could not be first, printer can't home at front edges without steelsheet on
    stsMoveZup,
    stsWait_axes,
    stsHeaters_noz_ena,
    stsHeaters_bed_ena,
    stsHeaters,
    stsWait_heaters,
    stsNet_status,
    stsFans_fine,
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

enum SelftestMask_t : uint64_t {
    stmNone = 0,
    stmFans = (uint64_t(1) << stsFans),
    stmWait_fans = (uint64_t(1) << stsWait_fans),
    stmXAxis = (uint64_t(1) << stsXAxis),
    stmYAxis = (uint64_t(1) << stsYAxis),
    stmZAxis = (uint64_t(1) << stsZAxis),
    stmMoveZup = (uint64_t(1) << stsMoveZup),
    stmXYAxis = (stmXAxis | stmYAxis),
    stmXYZAxis = (stmXAxis | stmYAxis | stmZAxis),
    stmWait_axes = (uint64_t(1) << stsWait_axes),
    stmHeaters_noz = ((uint64_t(1) << stsHeaters) | (uint64_t(1) << stsHeaters_noz_ena)),
    stmHeaters_bed = ((uint64_t(1) << stsHeaters) | (uint64_t(1) << stsHeaters_bed_ena)),
    stmHeaters = (stmHeaters_bed | stmHeaters_noz),
    stmWait_heaters = (uint64_t(1) << stsWait_heaters),
    stmSelftestStart = (uint64_t(1) << stsSelftestStart),
    stmSelftestStop = (uint64_t(1) << stsSelftestStop),
    stmNet_status = (uint64_t(1) << stsNet_status),
    stmShow_result = ((uint64_t(1) << stsShow_result) | (uint64_t(1) << stsResult_wait_user)),
    stmFullSelftest = (stmFans | stmXYZAxis | stmHeaters | stmNet_status | stmShow_result) | (uint64_t(1) << stsDidSelftestPass),
    stmWizardPrologue = (uint64_t(1) << stsPrologueAskRun) | (uint64_t(1) << stsPrologueAskRun_wait_user) | (uint64_t(1) << stsPrologueInfo) | (uint64_t(1) << stsPrologueInfo_wait_user) | (uint64_t(1) << stsPrologueInfoDetailed) | (uint64_t(1) << stsPrologueInfoDetailed_wait_user),
    stmEpilogue = (uint64_t(1) << stsEpilogue_nok) | (uint64_t(1) << stsEpilogue_nok_wait_user) | (uint64_t(1) << stsEpilogue_ok) | (uint64_t(1) << stsEpilogue_ok_wait_user),
    stmFirstLayer = (uint64_t(1) << stsFirstLayer),
    stmWizard = stmFullSelftest | stmWizardPrologue | stmEpilogue | stmFirstLayer,
    stmFans_fine = (uint64_t(1) << stsFans_fine),
};

// class representing whole self-test
class CSelftest : public AddSuper<ISelftest> {
public:
    CSelftest();

public:
    virtual bool IsInProgress() const override;
    virtual bool Start(uint64_t mask) override; // parent has no clue about SelftestMask_t
    virtual void Loop() override;
    virtual bool Abort() override;

protected:
    void phaseSelftestStart();
    void restoreAfterSelftest();
    virtual void next() override;
    virtual const char *get_log_suffix() override;
    void phaseShowResult();
    bool phaseWaitUser(PhasesSelftest phase);
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    selftest::IPartHandler *pPrintFan;
    selftest::IPartHandler *pHeatbreakFan;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    selftest::IPartHandler *pNozzle;
    selftest::IPartHandler *pBed;
    selftest::IPartHandler *pFirstLayer;

    SelftestResult_t m_result;
};
