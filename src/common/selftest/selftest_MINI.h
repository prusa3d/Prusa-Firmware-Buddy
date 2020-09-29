// selftest_MINI.h
#pragma once

#include <inttypes.h>
#include "ff.h"

#define SELFTEST_MAX_LOG_PRINTF 128
#define SELFTEST_LOOP_PERIODE   50

// forward declarations
class CSelftestPart;
class CSelftestPart_Fan;
class CSelftestPart_Axis;
class CSelftestPart_Heater;
class FSM_Holder;

typedef enum {
    stsIdle,
    stsStart,
    stsFans,
    stsXAxis,
    stsYAxis,
    stsZAxis,
    stsHeaters,
    stsFinish,
    stsFinished,
    //	stsAbort,
    stsAborted,
} SelftestState_t;

typedef enum {
    //	Start,
    stmNone = 0,
    stmFans = (1 << stsFans),
    stmXAxis = (1 << stsXAxis),
    stmYAxis = (1 << stsYAxis),
    stmZAxis = (1 << stsZAxis),
    stmXYZAxis = (stmXAxis | stmYAxis | stmZAxis),
    stmHeaters = (1 << stsHeaters),
    stmAll = (stmFans | stmXYZAxis | stmHeaters),
} SelftestMask_t;

// class representing whole self-test
class CSelftest {
    friend class CSelftestPart;
    friend class CSelftestPart_Fan;
    friend class CSelftestPart_Axis;
    friend class CSelftestPart_Heater;

public:
    CSelftest();

public:
    bool IsInProgress() const;

public:
    bool Start(SelftestMask_t mask);
    void Loop();
    bool Abort();

protected:
    void next();
    void log_open();
    void log_close();
    int log_printf(const char *fmt, ...);
    bool abort_part(CSelftestPart **ppart);

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    uint32_t m_Time;
    CSelftestPart_Fan *m_pFan0;
    CSelftestPart_Fan *m_pFan1;
    CSelftestPart_Axis *m_pXAxis;
    CSelftestPart_Axis *m_pYAxis;
    CSelftestPart_Axis *m_pZAxis;
    CSelftestPart_Heater *m_pHeater_Nozzle;
    CSelftestPart_Heater *m_pHeater_Bed;
    FSM_Holder *m_pFSM;
    FIL m_fil;
    bool m_filIsValid;
};

enum TestResult_t : uint8_t {
    sprUnknown,
    sprSkipped,
    sprPassed,
    sprFailed,
};

class CSelftestPart {
public:
    CSelftestPart();
    virtual ~CSelftestPart();

public:
    virtual bool IsInProgress() const = 0;

public:
    virtual bool Start() = 0;
    virtual bool Loop() = 0;
    virtual bool Abort() = 0;

public:
    float GetProgress() const;
    TestResult_t GetResult() const { return m_Result; };

protected:
    uint32_t m_StartTime;
    uint32_t m_EndTime;
    TestResult_t m_Result;
};

extern CSelftest Selftest;
