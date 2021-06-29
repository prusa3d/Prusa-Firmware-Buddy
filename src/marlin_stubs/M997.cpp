#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include "../common/sys.h"

void PrusaGcodeSuite::M997() {

    if (parser.seen('O')) {
        sys_fw_update_older_on_restart_enable();
    } else {
        sys_fw_update_enable();
    }

    queue.ok_to_send();
    HAL_Delay(10);
    NVIC_SystemReset();
}
