/**
 * @file
 */
#include <dirent.h>

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "libs/hex_print_routines.h"
#include "marlin_server.hpp"
#include "media.hpp"
#include "marlin_vars.hpp"
#include "str_utils.hpp"

struct TimeFlags {
    bool print_lfn : 1;
    bool print_timestamps : 1;
};

static void time_to_m20(const struct stat stat_buf, uint16_t *m20_date, uint16_t *m20_time) {
    time_t t = stat_buf.st_mtim.tv_sec;
    struct tm lt;
    localtime_r(&t, &lt);
    *m20_date = (lt.tm_year + 1900 - 1980) << 9 | (lt.tm_mon + 1) << 5 | lt.tm_mday;
    *m20_time = lt.tm_hour << 11 | lt.tm_min << 5;
    *m20_time |= int((lt.tm_sec - (lt.tm_sec % 2)) / 2);
}

static void print_file_info(const char *const prefix, const char *const path, const char *const long_path, const struct TimeFlags tf) {
    // we skip the /usb/ prefix on print as it is only used internally
    SERIAL_ECHO(&path[strlen_constexpr(prefix) + 1]);
    SERIAL_CHAR(' ');
    struct stat stat_buf;
    int rc = stat(path, &stat_buf);
    if (rc == 0) {
        SERIAL_ECHO(stat_buf.st_size);
        if (tf.print_timestamps) {
            uint16_t m20_date;
            uint16_t m20_time;
            time_to_m20(stat_buf, &m20_date, &m20_time);
            SERIAL_ECHOPGM(" 0x");
            print_hex_word(m20_date);
            print_hex_word(m20_time);
        }
        if (tf.print_lfn) {
            SERIAL_CHAR(' ');
            SERIAL_CHAR('"');
            SERIAL_ECHO(&long_path[strlen_constexpr(prefix) + 1]);
            SERIAL_CHAR('"');
        }
    }
    SERIAL_EOL();
}

static void print_listing(const char *const prefix, const char *const prefix_long, const struct TimeFlags tf) {
    DIR *dir;
    dir = opendir(prefix);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entry->d_name[0]) {
            char *path = nullptr;
            char *long_path = nullptr;
            int path_length = strlen_constexpr(prefix) + 1 + strlen_constexpr(entry->d_name) + 1;
            int long_path_length = strlen_constexpr(prefix_long) + 1 + strlen_constexpr(entry->lfn) + 1;
            path = (char *)malloc(path_length);
            snprintf(path, path_length, "%s/%s", prefix, entry->d_name);
            long_path = (char *)malloc(long_path_length);
            snprintf(long_path, long_path_length, "%s/%s", prefix_long, entry->lfn);
            if (entry->d_type != DT_DIR) {
                print_file_info(prefix, path, long_path, tf);
            }
            free(path);
            free(long_path);
        }
        closedir(dir);
    }
}

/** \addtogroup G-Codes
 * @{
 */

/**
 * M20 - List SD card on serial port
 */
void GcodeSuite::M20() {
    TimeFlags tf;
    if (parser.seen('L')) {
        tf.print_lfn = 1;
    }
    if (parser.seen('T')) {
        tf.print_timestamps = 1;
    }
    SERIAL_ECHOLNPGM(MSG_BEGIN_FILE_LIST);
    print_listing("/usb", "/usb", tf);
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
 *
 * - `U` - Unload filament when paused
 */
void GcodeSuite::M25() {
    if (parser.seen('U')) {
        marlin_server::print_pause_unload();
    } else {
        marlin_server::print_pause();
    }
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
    // TODO
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
