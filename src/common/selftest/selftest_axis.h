// selftest_axis.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"

typedef struct _selftest_axis_config_t {
    const char *partname;
    uint8_t axis;
} selftest_axis_config_t;

class CSelftestPart_Axis : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle,
        spsStart,
        spsWaitHome,
        spsFinish,
        spsFinished,
        spsAborted,
    };
    enum HomeState : uint8_t {
        hsNone,
        hsHommingInProgress,
        hsHommingFinished,
    };

public:
    CSelftestPart_Axis(const selftest_axis_config_t *pconfig);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

public:
    static void ResetHome();

protected:
    bool next();
    uint32_t estimate(const selftest_axis_config_t *pconfig);

protected:
    TestState m_State;
    const selftest_axis_config_t *m_pConfig;
    uint32_t m_Time;
    static HomeState m_Home;
};
