#include <dirent.h>

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../src/common/print_utils.hpp"
#include "marlin_server.hpp"
#include "media.hpp"
#include "marlin_vars.hpp"
#include <str_utils.hpp>

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
 * M20 - List SD card on serial port
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
 * M21 - Initialize SD card
 */
void GcodeSuite::M21() {
    // required for Octoprint / third party tools to simulate an inserted SD card when using USB
    if (marlin_vars()->media_inserted) {
        SERIAL_ECHOLNPGM(MSG_SD_CARD_OK);
    } else {
        SERIAL_ECHOLNPGM(MSG_SD_CARD_FAIL);
    }
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
    char namebuf[marlin_vars()->media_SFN_path.max_length()];
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
        marlin_vars()->media_SFN_path.set(namebuf);
        // Do not remove, needed for 3rd party tools such as octoprint to get notification about the gcode file being opened
        SERIAL_ECHOLNPAIR(MSG_SD_FILE_OPENED, parser.string_arg, MSG_SD_SIZE, fstats.st_size);
        SERIAL_ECHOLNPGM(MSG_SD_FILE_SELECTED);
    } else {
        SERIAL_ECHOLNPAIR(MSG_SD_OPEN_FILE_FAIL, parser.string_arg);
    }
}

/**
 * M24 - Start/resume SD print
 */
void GcodeSuite::M24() {
    if (media_print_get_state() == media_print_state_PAUSED) {
        marlin_server::print_resume();
    } else {
        marlin_server::print_start(marlin_vars()->media_SFN_path.get_ptr(), marlin_server::PreviewSkipIfAble::all);
    }
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

void GcodeSuite::M27() {
    if (parser.seen('C')) {
        SERIAL_ECHOPGM("Current file: ");
        SERIAL_ECHOLN(marlin_vars()->media_SFN_path.get_ptr());
    } else if (parser.seen('S')) {
        M27_handler::sd_auto_report_delay = parser.byteval('S');
    } else {
        M27_handler::print_sd_status();
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
