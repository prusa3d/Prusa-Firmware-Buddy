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

typedef struct _selftest_fan_config_t selftest_fan_config_t;
typedef struct _selftest_axis_config_t selftest_axis_config_t;
typedef struct _selftest_heater_config_t selftest_heater_config_t;

typedef enum {
    stsIdle,
    stsStart,
    stsFans,
    stsWait_fans,
    stsHome,
    stsXAxis,
    stsYAxis,
    stsZAxis,
    stsWait_axes,
    stsHeaters,
    stsWait_heaters,
    stsFans_fine,
    stsFinish,
    stsFinished,
    stsAborted,
} SelftestState_t;

typedef enum {
    stmNone = 0,
    stmFans = (1 << stsFans),
    stmWait_fans = (1 << stsWait_fans),
    stmHome = (1 << stsHome),
    stmXAxis = (1 << stsXAxis),
    stmYAxis = (1 << stsYAxis),
    stmZAxis = (1 << stsZAxis),
    stmXYAxis = (stmXAxis | stmYAxis),
    stmXYZAxis = (stmXAxis | stmYAxis | stmZAxis),
    stmWait_axes = (1 << stsWait_axes),
    stmHome_XYAxis = (stmXYAxis | stmHome),
    stmHome_XYZAxis = (stmXYZAxis | stmHome),
    stmHeaters = (1 << stsHeaters),
    stmWait_heaters = (1 << stsWait_heaters),
    stmAll = (stmFans | stmXYZAxis | stmHeaters),
    stmFans_fine = (1 << stsFans_fine),
} SelftestMask_t;

typedef enum _SelftestHomeState_t : uint8_t {
    sthsNone,
    sthsHommingInProgress,
    sthsHommingFinished,
} SelftestHomeState_t;

typedef union _SelftestResultEEprom_t {
    struct {
        uint8_t fan0 : 2;       // bit 0-1
        uint8_t fan1 : 2;       // bit 2-3
        uint8_t xaxis : 2;      // bit 4-5
        uint8_t yaxis : 2;      // bit 6-7
        uint8_t zaxis : 2;      // bit 8-9
        uint8_t nozzle : 2;     // bit 10-11
        uint8_t bed : 2;        // bit 12-13
        uint32_t reserved : 18; // bit 14-31
    };
    uint32_t ui32;
} SelftestResultEEprom_t;

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
    void phaseStart();
    bool phaseFans(const selftest_fan_config_t *pconfig_fan0, const selftest_fan_config_t *pconfig_fan1);
    bool phaseHome();
    bool phaseAxis(const selftest_axis_config_t *pconfig_axis, CSelftestPart_Axis **ppaxis, uint16_t fsm_phase, uint8_t progress_add, uint8_t progress_mul);
    bool phaseHeaters(const selftest_heater_config_t *pconfig_nozzle, const selftest_heater_config_t *pconfig_bed);
    void phaseFinish();
    bool phaseWait();

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
    SelftestHomeState_t m_HomeState;
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
    bool next();

protected:
    int m_State;
    uint32_t m_StartTime;
    uint32_t m_EndTime;
    TestResult_t m_Result;
};

extern CSelftest Selftest;
