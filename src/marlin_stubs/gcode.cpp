#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "PrusaGcodeSuite.hpp"

#include "M50.hpp"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 50:
            PrusaGcodeSuite::M50(); //selftest
            return true;
        }
        return false;
    case 'G':
        switch (parser.codenum) {
        case 26:
            PrusaGcodeSuite::G26();
            return true;
        case 162:
            PrusaGcodeSuite::G162();
            return true;
        }
        return false;
    default:
        return false;
    }
}
