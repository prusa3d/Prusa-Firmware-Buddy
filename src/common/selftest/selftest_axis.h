// selftest_axis.h
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_loop_result.hpp"
#include "selftest_axis_config.hpp"
#include "selftest_log.hpp"

namespace selftest {

class CSelftestPart_Axis {
    IPartHandler &state_machine;
    const AxisConfig_t &config;
    SelftestSingleAxis_t &rResult;
    uint32_t time_progress_start;
    uint32_t time_progress_estimated_end;
    uint8_t m_Step;
    uint32_t m_StartPos_usteps;
    uint16_t m_SGCount;
    uint32_t m_SGSum;
    uint8_t m_SGOrig_mask;
    void *m_pSGOrig_cb;
    static CSelftestPart_Axis *m_pSGAxis;

    void sg_sample(uint16_t sg);
    void sg_sampling_enable();
    void sg_sampling_disable();
    void phaseMove(int8_t dir);
    LoopResult wait(int8_t dir);
    static uint32_t estimate(const AxisConfig_t &config);
    static uint32_t estimate_move(float len_mm, float fr_mms);
    static void sg_sample_cb(uint8_t axis, uint16_t sg);
    void actualizeProgress() const;
    LogTimer log;
    int getDir() { return (m_Step % 2) ? -config.movement_dir : config.movement_dir; }

public:
    using Config = AxisConfig_t;

    static constexpr float EXTRA_LEN_MM = 10; // How far to move behind expected axis end

    CSelftestPart_Axis(IPartHandler &state_machine, const AxisConfig_t &config,
        SelftestSingleAxis_t &result);
    ~CSelftestPart_Axis();

    LoopResult stateHome();     ///< Enqueue homing and toolchange
    LoopResult stateWaitHome(); ///< Wait for homing and toolchange to finish
    LoopResult stateInitProgressTimeCalculation();
    LoopResult stateCycleMark() { return LoopResult::MarkLoop; }
    LoopResult stateMove();
    LoopResult stateMoveWaitFinish();
    LoopResult stateParkAxis();
};

extern const AxisConfig_t Config_XAxis;
extern const AxisConfig_t Config_YAxis;

};
