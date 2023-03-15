#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "eeprom.h"

#ifdef PRINT_CHECKING_Q_CMDS
/**
 * M862.4: Check firmware version
 */
void PrusaGcodeSuite::M862_4() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        SERIAL_ECHO_START();
        char temp_buf[sizeof("  M862.4 P0123456789")];
        snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.4 P%u"), eeprom_get_ui16(EEVAR_FW_VERSION));
        SERIAL_ECHO(temp_buf);
        SERIAL_EOL();
    }
}
#endif
