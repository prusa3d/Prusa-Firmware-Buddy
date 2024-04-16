/**
 * @file i_selftest.hpp
 * @brief Abstract parent of selftest
 */
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include "selftest_part.hpp"
#include "common/selftest/selftest_data.hpp"

#define SELFTEST_LOOP_PERIODE 50

// parent of class representing whole self-test
class ISelftest {

public:
    ISelftest();
    virtual ~ISelftest() = default;

    virtual bool IsInProgress() const = 0;
    virtual bool IsAborted() const = 0;
    virtual bool Start(const uint64_t test_mask, const ::selftest::TestData test_data) = 0;
    virtual void Loop() = 0;
    virtual bool Abort() = 0;
    uint32_t GetTime() { return m_Time; } // to synchronize time in selftest by loop ticks

protected:
    void phaseStart();
    void phaseFinish();
    bool phaseWait();

    virtual void next() = 0;
    bool abort_part(selftest::IPartHandler **ppart);

    uint32_t m_Time;
#if HAS_PHASE_STEPPING()
    std::optional<phase_stepping::EnsureDisabled> ph_disabler { std::nullopt };
#endif
};

// defined in child source file
ISelftest &SelftestInstance();
