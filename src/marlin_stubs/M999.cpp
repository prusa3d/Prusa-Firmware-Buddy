#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"

void PrusaGcodeSuite::M999() {
    queue.ok_to_send();
    HAL_Delay(10);
    NVIC_SystemReset();
}
