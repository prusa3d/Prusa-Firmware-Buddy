#pragma once
#include <stdint.h>

namespace modules {
namespace motion {

struct AxisSim {
    pos_t pos;
    pos_t targetPos;
    bool enabled;
    bool homed;
    bool stallGuard;
};

extern AxisSim axes[3];

void ReinitMotion();
bool PulleyEnabled();
void TriggerStallGuard(Axis axis);

} // namespace motion
} // namespace modules
