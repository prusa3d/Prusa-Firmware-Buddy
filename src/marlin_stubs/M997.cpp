#include "gcode/gcode.h"
#include "gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include <string.h>
#include "sys.h"
#include "data_exchange.hpp"

static void update_main_board(bool update_older, const char *sfn) {
    if (*sfn) { // Flash selected BBF
        data_exchange::set_reflash_bbf_sfn(sfn);
    } else {
        if (update_older) {
            data_exchange::fw_update_older_on_restart_enable();
        } else {
            data_exchange::fw_update_on_restart_enable();
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
    default:
        break;
    }
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M997: Perform in-application firmware update <a href="https://reprap.org/wiki/G-code#M997:_Perform_in-application_firmware_update">M997: Perform in-application firmware update</a>
 *
 *#### Usage
 *
 *    M997 [ O | S | B ]
 *
 *#### Parameters
 *
 * - `O` - Update older or same firmware on restart == force reflash == from menu
 * - `S` - Firmware module number(s), default 0
 *   - `0` - main firmware.
 *   - `1` - Reserved, check reprap wiki
 *   - `2` - Reserved, check reprap wiki
 *   - `3` - Reserved, check reprap wiki
 *   - `4` - Reserved, check reprap wiki
 * - `B` - Expansion board address, default 0
 *       - Currently unused, defined just to be reprap compatible
 *
 * - '/' - Selected BBF SFN (short file name)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M997() {

    char sfn[13] = { '\0' };
    const char *file_path_ptr = nullptr;
    static constexpr const char *const usb_str = "/usb/";
    size_t prefix_len = strlen(usb_str);

    if (parser.ulongval('S', 0) == 0) {
        if ((file_path_ptr = strstr(parser.string_arg, usb_str)) != nullptr) {
            if (*(file_path_ptr + prefix_len)) {
                strlcpy(sfn, file_path_ptr + prefix_len, sizeof(sfn));
            }
        }
    }

    // NOTICE: Keep in mind, that parser.seen('B') can be triggered by the filename in path of '/' parameter
    M997_no_parser(parser.ulongval('S', 0), parser.ulongval('B', 0), parser.seen('O'), sfn);
}

/** @}*/
