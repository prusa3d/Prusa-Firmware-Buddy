#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "PrusaGcodeSuite.hpp"

#include "M330.h"
#include "M50.hpp"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 50:
            PrusaGcodeSuite::M50(); //selftest
            return true;
#if defined(_DEBUG)
        case 330:
            PrusaGcodeSuite::M330();
            return true;
        case 331:
            PrusaGcodeSuite::M331();
            return true;
        case 332:
            PrusaGcodeSuite::M332();
            return true;
        case 333:
            PrusaGcodeSuite::M333();
            return true;
        case 334:
            PrusaGcodeSuite::M334();
            return true;
#endif // _DEBUG
        case 1400:
            PrusaGcodeSuite::M1400();
            return true;
        default:
            return false;
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
