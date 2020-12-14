// selftest_axis.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"

struct selftest_axis_config_t {
    const char *partname;
    float length;
    const float *fr_table;
    float length_min;
    float length_max;
    uint8_t axis;
    uint8_t steps;
    int8_t dir;
};

class CSelftestPart_Axis : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle = 0,
        spsStart,
        spsWaitHome,
        spsMoveFwd,
        spsWaitFwd,
        spsMoveRev,
        spsWaitRev,
        spsFinish,
        spsFinished,
        spsAborted,
        spsFailed,
    };

public:
    CSelftestPart_Axis(const selftest_axis_config_t &config);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

public:
    uint8_t getFSMState();

protected:
    void phaseMove(int8_t dir);
    bool phaseWait(int8_t dir);

protected:
    static uint32_t estimate(const selftest_axis_config_t &config);
    static uint32_t estimate_move(float len_mm, float fr_mms);
    static void sg_sample_cb(uint8_t axis, uint16_t sg);

protected:
    void sg_sample(uint16_t sg);
    void sg_sampling_enable();
    void sg_sampling_disable();

protected:
    const selftest_axis_config_t &m_config;
    uint32_t m_Time;
    uint8_t m_Step;
    uint32_t m_StartPos_usteps;
    uint16_t m_SGCount;
    uint32_t m_SGSum;
    uint8_t m_SGOrig_mask;
    void *m_pSGOrig_cb;
    static CSelftestPart_Axis *m_pSGAxis;
};
