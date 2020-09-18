#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "G162.hpp"
#include "D666.hpp"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'M': //idiotic parser does not see 'D'
        switch (parser.codenum) {
        case 50:
            PrusaGcodeSuite::D666(); //selftest
            return true;
        }
        return false;
    case 'G':
        switch (parser.codenum) {
        case 162:
            PrusaGcodeSuite::G162();
            return true;
        }
        return false;
    default:
        return false;
    }
}
