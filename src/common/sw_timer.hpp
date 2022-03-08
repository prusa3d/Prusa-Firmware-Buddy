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
    bool IsOver(const T now) { return now - start_time >= timeout_; }
    bool RestartIfIsOver(const T now) {
        bool res = IsOver(now);
        if (res)
            Restart(now);
        return res;
    }

private:
    T start_time = 0;
    T timeout_ = 0;
};
