#pragma once

#include <Marlin/src/module/planner.h>

namespace mapi {

/// Limit max X/Y acceleration to a value, restore the previous values when destroyed.
class AccelerationLimiter {
public:
    explicit AccelerationLimiter(float max_acceleration_mmss)
        : previous_x(planner.user_settings.max_acceleration_mm_per_s2[X_AXIS])
        , previous_y(planner.user_settings.max_acceleration_mm_per_s2[Y_AXIS]) {
        planner.set_max_acceleration(X_AXIS, max_acceleration_mmss);
        planner.set_max_acceleration(Y_AXIS, max_acceleration_mmss);
    }
    ~AccelerationLimiter() {
        planner.set_max_acceleration(X_AXIS, previous_x);
        planner.set_max_acceleration(Y_AXIS, previous_y);
    }
    AccelerationLimiter(const AccelerationLimiter &) = delete;
    AccelerationLimiter &operator=(const AccelerationLimiter &) = delete;

private:
    const float previous_x, previous_y;
};

} // namespace mapi
