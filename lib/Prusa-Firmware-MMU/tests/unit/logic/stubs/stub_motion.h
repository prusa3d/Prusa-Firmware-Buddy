#pragma once
#include <stdint.h>
#include <deque>

namespace modules {
namespace motion {

struct AxisSim {
    pos_t pos;
    bool enabled;
    bool homed;
    bool stallGuard;
    int8_t sg_thrs;
    std::deque<pos_t> plannedMoves;
};

extern AxisSim axes[3];

void ReinitMotion();
bool PulleyEnabled();
void TriggerStallGuard(Axis axis);
pos_t AxisNearestTargetPos(Axis axis);

} // namespace motion
} // namespace modules
