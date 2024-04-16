#pragma once
/*
 * Generic timer class.
 *
 * Can cause premature timeout after counter overflow.
 */

#include <cstdint>

#include "timing.h"

template <typename T>
class Sw_Timer {
public:
    Sw_Timer(const T timeout = 0) { SetTimer(timeout); }
    void Restart(const T now) { start_time = now; }
    void SetTimer(const T timeout) { timeout_ = timeout; }
    T GetTimer() { return timeout_; }
    bool IsOver(const T now) { return now - start_time >= timeout_; }
    T Passed(const T now) { return now - start_time; }
    // Can overflow if unsigned int is used
    T Remains(const T now) { return timeout_ - Passed(now); }
    bool RestartIfIsOver(const T now) {
        bool res = IsOver(now);
        if (res) {
            Restart(now);
        }
        return res;
    }

private:
    T start_time = 0;
    T timeout_ = 0;
};
