#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.h"

// M20 - List SD card
void GcodeSuite::M20() {
    //TODO
}

// M21 - Initialize SD card
void GcodeSuite::M21() {
    //TODO
}

// M22 - Release SD card
void GcodeSuite::M22() {
    //TODO
}

// M23 - Select SD file
void GcodeSuite::M23() {
    //TODO
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
    //TODO
}

// M27 - Report SD print status
void GcodeSuite::M27() {
    //TODO
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

// M33 - Get the long name for an SD card file or folder
// M33 - Stop and Close File and save restart.gcode
// M34 - Set SD file sorting options
