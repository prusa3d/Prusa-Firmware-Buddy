#include <dirent.h>

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../src/common/print_utils.hpp"
#include "marlin_server.hpp"
#include "media.hpp"
#include "marlin_vars.hpp"
#include <str_utils.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 * M20 - List SD card on serial port
 */
void GcodeSuite::M20() {
    SERIAL_ECHOLNPGM(MSG_BEGIN_FILE_LIST);
    DIR *dir;
    dir = opendir("/usb/");
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entry->d_name[0]) {
            SERIAL_ECHOLN(entry->d_name);
        }
        closedir(dir);
    }
    SERIAL_ECHOLNPGM(MSG_END_FILE_LIST);
}

/**
 * M21 - Initialize SD card
 */
void GcodeSuite::M21() {
    // not necessary - empty implementation
}

/**
 * M22 - Release SD card
 */
void GcodeSuite::M22() {
    // not necessary - empty implementation
}

/**
 * M23 - Select SD file
 */
void GcodeSuite::M23() {
    // Simplify3D includes the size, so zero out all spaces (#7227)
    for (char *fn = parser.string_arg; *fn; ++fn) {
        if (*fn == ' ') {
            *fn = '\0';
        }
    }
    marlin_vars()->media_SFN_path.set(parser.string_arg);
    // Do not remove. Used by third party tools to detect that a file has been selected
    SERIAL_ECHOLNPGM(MSG_SD_FILE_SELECTED);
}

/**
 * M24 - Start/resume SD print
 */
void GcodeSuite::M24() {
    marlin_server::print_resume();
}

/**
 * @brief Pause SD print
 */
void GcodeSuite::M25() {
    marlin_server::print_pause();
}

/**
 * M26 - Set SD position
 *
 * ## Parameters
 *
 * - `S` - [value] Specific position
 */
void GcodeSuite::M26() {
    if ((media_get_state() == media_state_INSERTED) && parser.seenval('S')) {
        media_print_set_position(parser.value_ulong());
    }
}

/**
 * M27 - Report SD print status on serial port
 *
 * ## Parameters
 *
 * - `C` - Report current file's short file name instead
 */
void GcodeSuite::M27() {
    if (parser.seen('C')) {
        SERIAL_ECHOPGM("Current file: ");
        SERIAL_ECHOLN(marlin_vars()->media_SFN_path.get_ptr());
    } else {
        if (media_print_get_state() != media_print_state_NONE) {
            SERIAL_ECHOPGM(MSG_SD_PRINTING_BYTE);
            SERIAL_ECHO(media_print_get_position());
            SERIAL_CHAR('/');
            SERIAL_ECHOLN(media_print_get_size());
        } else {
            SERIAL_ECHOLNPGM(MSG_SD_NOT_PRINTING);
        }
    }
}

/** @}*/

// M28 - Begin write to SD card
void GcodeSuite::M28() {
    // TODO
}

// M29 - Stop writing to SD card
void GcodeSuite::M29() {
    // TODO
}

// M30 - Delete a file on the SD card
void GcodeSuite::M30() {
    ArrayStringBuilder<FF_MAX_LFN> filepath;
    filepath.append_printf("/usb/%s", parser.string_arg);
    DeleteResult result = DeleteResult::GeneralError;
    if (filepath.is_ok()) {
        result = remove_file(filepath.str());
    }
    SERIAL_ECHOPGM(result == DeleteResult::Success ? "File deleted:" : "Deletion failed:");
    SERIAL_ECHO(parser.string_arg);
    SERIAL_ECHOLN(".");
}

// M32 - Select file and start SD print
void GcodeSuite::M32() {
    M23();
    M24();
}

//
// M33 - Get the long name for an SD card file or folder
// M33 - Stop and Close File and save restart.gcode
// M34 - Set SD file sorting options
