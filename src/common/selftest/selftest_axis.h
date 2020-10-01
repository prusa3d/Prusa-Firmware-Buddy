// selftest_axis.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"

typedef struct _selftest_axis_config_t {
    const char *partname;
    uint8_t axis;
    uint8_t steps;
    int8_t dir;
    float length;
    const float *fr_table;
} selftest_axis_config_t;

class CSelftestPart_Axis : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle,
        spsStart,
        spsWaitHome,
        spsMoveFwd,
        spsWaitFwd,
        spsMoveRev,
        spsWaitRev,
        spsFinish,
        spsFinished,
        spsAborted,
    };

public:
    CSelftestPart_Axis(const selftest_axis_config_t *pconfig);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

protected:
    void phaseMove(int8_t dir);
    bool phaseWait(int8_t dir);

protected:
    bool next();
    static uint32_t estimate(const selftest_axis_config_t *pconfig);
    static uint32_t estimate_move(float len_mm, float fr_mms);

protected:
    TestState m_State;
    const selftest_axis_config_t *m_pConfig;
    uint32_t m_Time;
    uint8_t m_Step;
    uint32_t m_StartPos_usteps;
};
