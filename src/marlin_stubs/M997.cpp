#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include <option/has_selftest.h>
#include <string.h>
#if HAS_SELFTEST()
    #include "selftest_esp.hpp"
#endif // HAS_SELFTEST
#include "../common/sys.h"

static void update_main_board(bool update_older, const char *sfn) {
    if (sfn != nullptr) { // Flash selected BBF
        sys_set_reflash_bbf_sfn(sfn);
    } else {
        if (update_older) {
            sys_fw_update_older_on_restart_enable();
        } else {
            sys_fw_update_enable();
        }
    }

    queue.ok_to_send();
    HAL_Delay(10);
    NVIC_SystemReset();
}

static void M997_no_parser(uint module_number, [[maybe_unused]] uint address, bool force_update_older, const char *sfn) {
    switch (module_number) {
    case 0:
        update_main_board(force_update_older, sfn);
        break;
#if HAS_SELFTEST()
    case 1:
        update_esp(force_update_older);
        break;
#endif // HAS_SELFTEST
    default:
        break;
    }
}

/**
 * Perform in-application firmware update
 *
 * ## Parameters
 *
 * - `O` - Update older or same firmware on restart == force reflash == from menu
 * - `S` - Firmware module number(s), default 0
 *       - 0 - main firmware.
 *       - 1 - WiFi module firmware
 *       - 2 - 4 - Reserved, check reprap wiki
 * - `B` - Expansion board address, default 0
 *       - Currently unused, defined just to be reprap compatible
 *
 * - '/' - Selected BBF SFN (short file name)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M997() {

    char sfn[13] = { 0 };
    const char *file_path_ptr = nullptr;
    if (parser.ulongval('S', 0) == 0) { // Reflashing main FW
        file_path_ptr = strstr(parser.string_arg, "/");
        if (file_path_ptr != nullptr)
            strlcpy(sfn, file_path_ptr + 1, 13);
    }

    M997_no_parser(parser.ulongval('S', 0), parser.ulongval('B', 0), parser.seen('O'), sfn);
}
