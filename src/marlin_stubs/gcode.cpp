#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "PrusaGcodeSuite.hpp"
#include "client_fsm_types.h"
#include "marlin_server.hpp"
#include "G26.hpp"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'G':
        switch (parser.codenum) {
        case 26:
            G26();
        case 162:
            PrusaGcodeSuite::G162();
            return true;
        }
        return false;
    default:
        return false;
    }
}
