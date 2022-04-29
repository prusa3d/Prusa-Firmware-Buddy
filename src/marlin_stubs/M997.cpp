#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include "selftest_esp.hpp"
#include "../common/sys.h"

static void update_main_board(bool update_older) {
    if (update_older) {
        sys_fw_update_older_on_restart_enable();
    } else {
        sys_fw_update_enable();
    }

    queue.ok_to_send();
    HAL_Delay(10);
    NVIC_SystemReset();
}

static void M997_no_parser(uint module_number, uint address, bool force_update_older) {
    switch (module_number) {
    case 0:
        update_main_board(force_update_older);
        break;
    case 1:
        update_esp(force_update_older);
        break;
    default:
        break;
    }
}

/**
 * M997: Perform in-application firmware update
 *
 *  O          - Update older or same firmware on restart == force reflash == from menu
 *  S<number>  - Firmware module number(s), default 0
 *             - 0 - main firmware.
 *             - 1 - WiFi module firmware
 *             - 2 - 4 - Reserved, check reprap wiki
 *  B<address> - Expansion board address, default 0
 *             - Currently unused, defined just to be reprap compatible
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M997() {
    M997_no_parser(parser.ulongval('S', 0), parser.ulongval('B', 0), parser.seen('O'));
}
