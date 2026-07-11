#include <dirent.h>

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/gcode/lcd/M73_PE.h"
#include "../src/common/print_utils.hpp"
#include "marlin_server.hpp"
#include <usb_host.h>
#include "marlin_vars.hpp"
#include <common/directory.hpp>
#include <utils/string_builder.hpp>
#include "gcode_reader_restore_info.hpp"

struct ListControl {
    bool print_lfn : 1;
    bool print_time : 1;
    uint8_t recursion_count;
    time_t tz_offset_seconds;
};

// Forward reference (for recursion)
static void list_files(const char *const dir_path, struct ListControl *lc);
// Depends on stack size/RAM, etc
// Set to 0 to disallow recursion, Marlin MAX is 6
#define MAX_RECURSION_DEPTH 4
// Device root name for FatFS
#define ROOT_PREFIX "/usb"

// Routine to output single dirent info in Marlin M20 format
// Note: Use of 'alloca' prohibits inline
static void __attribute__((noinline)) list_single_entry(const char *dir_path, struct dirent *entry, struct ListControl *lc) {
    // Construct path to sub-dir.
    int len = strlen(dir_path) + strlen(entry->d_name) + 2;
    char *path = reinterpret_cast<char *>(alloca(len));
    strcpy(path, dir_path);
    strcat(path, "/");
    strcat(path, entry->d_name);

    if (entry->d_type != DT_DIR) {
        struct stat fstats;

        // Hide ROOT_PREFIX
        SERIAL_ECHO(&path[sizeof(ROOT_PREFIX) - 1]);
        int rc = stat(path, &fstats);
        if (rc == 0) {
            SERIAL_ECHOPAIR(PSTR(" "), fstats.st_size);
            if (lc->print_time) {
                struct tm lt;
                time_t t = fstats.st_mtim.tv_sec + lc->tz_offset_seconds;
                localtime_r(&t, &lt);
                // M20 Date in high-word
                t = ((lt.tm_year + 1900 - 1980) << 9 | (lt.tm_mon + 1) << 5 | lt.tm_mday) << 16;
                // M20 Time in low-word
                t |= lt.tm_hour << 11 | lt.tm_min << 5 | int((lt.tm_sec - (lt.tm_sec % 2)) / 2);
                SERIAL_ECHOPGM(" 0x");
                SERIAL_PRINT((unsigned int)t, HEX);
            }
            if (lc->print_lfn) {
                SERIAL_ECHOPAIR(PSTR(" \""), entry->lfn, PSTR("\""));
            }
        }
        SERIAL_EOL();
    } else {
        // Check recursion depth
        if (lc->recursion_count == 0) {
            return;
        }

        if (lc->print_lfn) {
            SERIAL_ECHOPAIR(PSTR("DIR_ENTER: "), &path[sizeof(ROOT_PREFIX) - 1], PSTR("/ \""), entry->lfn, PSTR("\""));
            SERIAL_EOL();
        }
        // List sub-directory calling 'list_files' recursively
        lc->recursion_count--;
        list_files(path, lc);
        lc->recursion_count++;
        if (lc->print_lfn) {
            SERIAL_ECHOLNPGM("DIR_EXIT");
        }
    }
    return;
}

// List all files in a single directory
// Note that this function is called recursively
static void list_files(const char *const dir_path, struct ListControl *lc) {
    DIR *dir;
    dir = opendir(dir_path);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entry->d_name[0]) {
            // Output single directory entry or perform recursion into sub-dir
            list_single_entry(dir_path, entry, lc);
        }
        closedir(dir);
    }
    return;
}

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
    ListControl lc;
    if (parser.seen('L')) {
        lc.print_lfn = 1;
    }
    if (parser.seen('T')) {
        lc.print_time = 1;
    }
    // Get timezone offset to report local filetime
    lc.tz_offset_seconds = time_tools::calculate_total_timezone_offset_minutes() * 60;
    lc.recursion_count = MAX_RECURSION_DEPTH;
    SERIAL_ECHOLNPGM(MSG_BEGIN_FILE_LIST);
    list_files(ROOT_PREFIX, &lc);
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
    // required for Octoprint / third party tools to simulate an inserted SD card when using USB
    if (marlin_vars().media_inserted.get()) {
        SERIAL_ECHOLNPGM(MSG_SD_CARD_OK);
    } else {
        SERIAL_ECHOLNPGM(MSG_SD_CARD_FAIL);
    }
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
    char namebuf[FILE_PATH_BUFFER_LEN];
    // Simplify3D includes the size, terminate name at first space
    char *idx = strchr(parser.string_arg, ' ');
    if (idx) {
        *idx = '\0';
    }
    // Need to prepend root device name
    strcpy(namebuf, PSTR(ROOT_PREFIX));
    strncpy(&namebuf[sizeof(ROOT_PREFIX) - 1], parser.string_arg, sizeof(namebuf) - sizeof(ROOT_PREFIX));

    // Do not remove. Used by third party tools to detect that a file has been selected
    SERIAL_ECHO_START();
    SERIAL_ECHOPAIR(PSTR("Now doing file: "), parser.string_arg);
    SERIAL_EOL();

    struct stat fstats;
    int rc = stat(namebuf, &fstats);
    if (rc == 0) {
        marlin_vars().media_SFN_path.set(namebuf);
        // Do not remove, needed for 3rd party tools such as octoprint to get notification about the gcode file being opened
        SERIAL_ECHOLNPAIR(MSG_SD_FILE_OPENED, parser.string_arg, MSG_SD_SIZE, fstats.st_size);
        SERIAL_ECHOLNPGM(MSG_SD_FILE_SELECTED);
    } else {
        SERIAL_ECHOLNPAIR(MSG_SD_OPEN_FILE_FAIL, parser.string_arg);
    }
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
    // Do nothing if busy
    if (marlin_server::printer_idle()) {
        if (marlin_vars().print_state.get() == marlin_server::State::Paused) {
            marlin_server::print_resume();
        } else {
            SERIAL_ECHOLNPGM("**M24 print_start called");
            oProgressData.mInit();
            marlin_server::print_start(marlin_vars().media_SFN_path.get_ptr(), GCodeReaderPosition(), marlin_server::PreviewSkipIfAble::all);
        }
    }
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
uint32_t M27_handler::sd_auto_report_delay = 0;

void M27_handler::print_sd_status() {
    if (marlin_vars().print_state.get() != marlin_server::State::Printing) {
        SERIAL_ECHOPGM(MSG_SD_PRINTING_BYTE);
        SERIAL_ECHO(marlin_vars().media_position);
        SERIAL_CHAR('/');
        SERIAL_ECHOLN(marlin_vars().media_size_estimate);
    } else {
        SERIAL_ECHOLNPGM(MSG_SD_NOT_PRINTING);
    }
}

void GcodeSuite::M27() {
    if (parser.seen('C')) {
        SERIAL_ECHOPGM("Current file: ");
        SERIAL_ECHOLN(marlin_vars().media_SFN_path.get_ptr());
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

//
// ### M33 - Get the long name for an SD card file or folder
// ### M33 - Stop and Close File and save restart.gcode
// ### M34 - Set SD file sorting options
