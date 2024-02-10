#include <dirent.h>

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"
#include <usb_host.h>
#include "marlin_vars.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M20 - List SD card on serial port  <a href="https://reprap.org/wiki/G-code#M20:_List_SD_card">M20: List SD card</a>
 *
 *#### Usage
 *
 *    M20
 *
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
 *### M21 - Initialize SD card <a href="https://reprap.org/wiki/G-code#M21:_Initialize_SD_card">M21: Initialize SD card</a>
 *
 *#### Usage
 *
 *    M21
 *
 */
void GcodeSuite::M21() {
    // not necessary - empty implementation
}

/**
 *### M22: Release SD card <a href="https://reprap.org/wiki/G-code#M22:_Release_SD_card">M22: Release SD card</a>
 *
 *#### Usage
 *
 *    M22
 *
 */
void GcodeSuite::M22() {
    // not necessary - empty implementation
}

/**
 *### M23 - Select SD file <a href="https://reprap.org/wiki/G-code#M23:_Select_SD_file">M23: Select SD file</a>
 *
 *#### Usage
 *
 *    M23 [ filename ]
 *
 */
void GcodeSuite::M23() {
    // Simplify3D includes the size, so zero out all spaces (#7227)
    for (char *fn = parser.string_arg; *fn; ++fn) {
        if (*fn == ' ') {
            *fn = '\0';
        }
    }
    marlin_vars().media_SFN_path.set(parser.string_arg);
    // Do not remove. Used by third party tools to detect that a file has been selected
    SERIAL_ECHOLNPGM(MSG_SD_FILE_SELECTED);
}

/**
 *### M24 - Start/resume SD print <a href="https://reprap.org/wiki/G-code#M24:_Start.2Fresume_SD_print">M24: Start/resume SD print</a>
 *
 *#### Usage
 *
 *    M24
 *
 */
void GcodeSuite::M24() {
    marlin_server::print_resume();
}

/**
 *### Pause SD print <a href="https://reprap.org/wiki/G-code#M25:_Pause_SD_print">M25: Pause SD print</a>
 *
 *#### Usage
 *
 *    M25
 *
 */
void GcodeSuite::M25() {
    marlin_server::print_pause();
}

/**
 *### M26 - Set SD position <a href="https://reprap.org/wiki/G-code#M26:_Set_SD_position">M26: Set SD position</a>
 *
 *#### Usage
 *
 *    M26 [ S ]
 *
 *
 *#### Parameters
 *
 *  - `S` - Specific position
 *
 */
void GcodeSuite::M26() {
    if (usb_host::is_media_inserted() && parser.seenval('S')) {
        marlin_server::set_media_position(parser.value_ulong());
    }
}

uint32_t M27_handler::sd_auto_report_delay = 0;

void M27_handler::print_sd_status() {
    if (media_print_get_state() != media_print_state_NONE) {
        SERIAL_ECHOPGM(MSG_SD_PRINTING_BYTE);
        SERIAL_ECHO(media_print_get_position());
        SERIAL_CHAR('/');
        SERIAL_ECHOLN(media_print_get_size());
    } else {
        SERIAL_ECHOLNPGM(MSG_SD_NOT_PRINTING);
    }
}

/**
 *### M27 - Report SD print status on serial port <a href="https://reprap.org/wiki/G-code#M27:_Report_SD_print_status">M27: Report SD print status</a>
 *
 *#### Usage
 *
 *    M37 [ C ]
 *
 *#### Parameters
 *
 *  - `C` - Report current file's short file name instead
 *
 */
void GcodeSuite::M27() {
    if (parser.seen('C')) {
        SERIAL_ECHOPGM("Current file: ");
        SERIAL_ECHOLN(marlin_vars().media_SFN_path.get_ptr());

    } else if (marlin_server::is_printing_state(marlin_vars().print_state.get())) {
        SERIAL_ECHOPGM(MSG_SD_PRINTING_BYTE);
        SERIAL_ECHO(marlin_vars().media_position.get());
        SERIAL_CHAR('/');
        SERIAL_ECHOLN(marlin_vars().media_size_estimate.get());
    } else if (parser.seen('S')) {
        M27_handler::sd_auto_report_delay = parser.byteval('S');
    } else {
        M27_handler::print_sd_status();
    }
}

/**
 *### M32 - Select file and start SD print <a href="https://reprap.org/wiki/G-code#M32:_Select_file_and_start_SD_print">M32: Select file and start SD print</a>
 *
 *#### Usage
 *
 *    M32 [ filename ]
 *
 */
void GcodeSuite::M32() {
    M23();
    M24();
}

/** @}*/

/**
 *### M28 - Start SD write <a href="https://reprap.org/wiki/G-code#M28:_Begin_write_to_SD_card">M28: Begin write to SD card</a>
 *
 *#### Usage
 *
 *   M28 []
 */
void GcodeSuite::M28() {
    // TODO
}

/**
 *### M29 - Stop SD write <a href="https://reprap.org/wiki/G-code#M29:_Stop_writing_to_SD_card">M29: Stop writing to SD card</a>
 *
 *Stops writing to the SD file signaling the end of the uploaded file. It is processed very early and it's not written to the card.
 *
 *#### Usage
 *
 *   M29 []
 */
void GcodeSuite::M29() {
    // TODO
}

/**
 *### M30 - Delete file <a href="https://reprap.org/wiki/G-code#M30:_Delete_a_file_on_the_SD_card">M30: Delete a file on the SD card</a>
 *
 *#### Usage
 *
 *    M30 [filename]
 */
void GcodeSuite::M30() {
    // TODO
}

//
// ### M33 - Get the long name for an SD card file or folder
// ### M33 - Stop and Close File and save restart.gcode
// ### M34 - Set SD file sorting options
