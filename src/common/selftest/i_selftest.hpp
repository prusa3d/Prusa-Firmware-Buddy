/**
 * @file i_selftest.hpp
 * @brief Abstract parent of selftest
 */
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include "selftest_part.hpp"

#define SELFTEST_MAX_LOG_PRINTF 128
#define SELFTEST_LOOP_PERIODE   50

// parent of class representing whole self-test
class ISelftest {

public:
    ISelftest();
    virtual ~ISelftest() = default;

    virtual bool IsInProgress() const = 0;
    virtual bool IsAborted() const = 0;
    virtual bool Start(const uint64_t test_mask, const uint8_t tool_mask) = 0;
    virtual void Loop() = 0;
    virtual bool Abort() = 0;
    uint32_t GetTime() { return m_Time; } // to synchronize time in selftest by loop ticks
    void log_printf(const char *fmt, ...); // return 1 on succeessful fprint

protected:
    virtual void phaseStart();
    virtual void phaseFinish();
    bool phaseWait();

    virtual void next() = 0;
    void log_open();
    virtual const char *get_log_suffix() = 0;
    void log_close();
    bool abort_part(selftest::IPartHandler **ppart);

    uint32_t m_Time;
    FILE *m_USBLog_fp;
};

// defined in child source file
ISelftest &SelftestInstance();
