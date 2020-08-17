#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.h"
#include "media.h"
#include "ff.h"

// M20 - List SD card
void GcodeSuite::M20() {
    SERIAL_ECHOLNPGM(MSG_BEGIN_FILE_LIST);
    DIR dir = { 0 };
    FRESULT result = f_opendir(&dir, "/");
    if (result == FR_OK) {
        FILINFO current_finfo = { 0 };
        result = f_findfirst(&dir, &current_finfo, "", "*.gco*");
        while (result == FR_OK && current_finfo.fname[0]) {
            SERIAL_ECHOLN(current_finfo.altname);
            result = f_findnext(&dir, &current_finfo);
        }
    }
    f_closedir(&dir);
    SERIAL_ECHOLNPGM(MSG_END_FILE_LIST);
}

// M21 - Initialize SD card
void GcodeSuite::M21() {
    //not necessary - empty implementation
}

// M22 - Release SD card
void GcodeSuite::M22() {
    //not necessary - empty implementation
}

// M23 - Select SD file
void GcodeSuite::M23() {
    // Simplify3D includes the size, so zero out all spaces (#7227)
    for (char *fn = parser.string_arg; *fn; ++fn)
        if (*fn == ' ')
            *fn = '\0';
    strlcpy(media_print_filepath(), parser.string_arg, MEDIA_PRINT_FILEPATH_SIZE);
}

// M24 - Start/resume SD print
void GcodeSuite::M24() {
    marlin_server_print_resume();
}

// M25 - Pause SD print
void GcodeSuite::M25() {
    marlin_server_print_pause();
}

// M26 - Set SD position
void GcodeSuite::M26() {
    if ((media_get_state() == media_state_INSERTED) && parser.seenval('S'))
        media_print_set_position(parser.value_ulong());
}

// M27 - Report SD print status
void GcodeSuite::M27() {
    if (parser.seen('C')) {
        SERIAL_ECHOPGM("Current file: ");
        SERIAL_ECHOLN(media_print_filepath());
    } else {
        if ((media_print_get_state() == media_print_state_PRINTING) || (media_print_get_state() == media_print_state_PAUSED)) {
            SERIAL_ECHOPGM(MSG_SD_PRINTING_BYTE);
            SERIAL_ECHO(media_print_get_position());
            SERIAL_CHAR('/');
            SERIAL_ECHOLN(media_print_get_size());
        } else
            SERIAL_ECHOLNPGM(MSG_SD_NOT_PRINTING);
    }
}

// M28 - Begin write to SD card
void GcodeSuite::M28() {
    //TODO
}

// M29 - Stop writing to SD card
void GcodeSuite::M29() {
    //TODO
}

// M30 - Delete a file on the SD card
void GcodeSuite::M30() {
    //TODO
}

// M32 - Select file and start SD print
void GcodeSuite::M32() {
    //TODO
}

//
// M33 - Get the long name for an SD card file or folder
// M33 - Stop and Close File and save restart.gcode
// M34 - Set SD file sorting options
