#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "G162.hpp"
#include "ScreenHandler.hpp"
#include "ScreenFirstLayer.hpp"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'G':
        switch (parser.codenum) {
        case 26:
            Screens::Access()->Open(ScreenFactory::Screen<ScreenFirstLayer>);
        case 162:
            PrusaGcodeSuite::G162();
            return true;
        }
        return false;
    default:
        return false;
    }
}
