// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"

typedef struct _selftest_heater_config_t {
    const char *partname;
    uint8_t heater;
} selftest_heater_config_t;

class CSelftestPart_Heater : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle,
        spsStart,
        spsWait,
        spsFinish,
        spsFinished,
        spsAborted,
    };

public:
    CSelftestPart_Heater(const selftest_heater_config_t *pconfig);

public:
    bool IsInProgress() const;

public:
    bool Start();
    bool Loop();
    bool Abort();

protected:
    bool next();
    static uint32_t estimate(const selftest_heater_config_t *pconfig);

protected:
    TestState m_State;
    const selftest_heater_config_t *m_pConfig;
    uint32_t m_Time;
};
