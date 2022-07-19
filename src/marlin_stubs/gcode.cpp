#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "PrusaGcodeSuite.hpp"

#include "M330.h"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 50:
            PrusaGcodeSuite::M50(); //selftest
            return true;
        case 300:
            PrusaGcodeSuite::M300();
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
        case 505:
            PrusaGcodeSuite::M505();
            return true;
        case 650:
            PrusaGcodeSuite::M650();
            return true;
        case 704:
            PrusaGcodeSuite::M704();
            return true;
        case 705:
            PrusaGcodeSuite::M705();
            return true;
        case 706:
            PrusaGcodeSuite::M706();
            return true;
        case 707:
            PrusaGcodeSuite::M707();
            return true;
        case 708:
            PrusaGcodeSuite::M708();
            return true;
        case 709:
            PrusaGcodeSuite::M709();
            return true;
        case 930:
            PrusaGcodeSuite::M930();
            return true;
        case 931:
            PrusaGcodeSuite::M931();
            return true;
        case 932:
            PrusaGcodeSuite::M932();
            return true;
        case 997:
            PrusaGcodeSuite::M997();
            return true;
        case 999:
            if (parser.seen('R')) {
                PrusaGcodeSuite::M999();
                return true;
            } else {
                return false;
            }
        case 1587:
            PrusaGcodeSuite::M1587();
            return true;
        case 1600:
            PrusaGcodeSuite::M1600();
            return true;
        case 1700:
            PrusaGcodeSuite::M1700();
            return true;
        case 1701:
            PrusaGcodeSuite::M1701();
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
        case 163:
            PrusaGcodeSuite::G163();
            return true;
        }
        return false;
    default:
        return false;
    }
}

//weak g-codes to prevent ugly preprocessor
//TODO write error message to log if used
void __attribute__((weak)) PrusaGcodeSuite::M50() {}
void __attribute__((weak)) PrusaGcodeSuite::M650() {}
void __attribute__((weak)) PrusaGcodeSuite::M704() {}
void __attribute__((weak)) PrusaGcodeSuite::M705() {}
void __attribute__((weak)) PrusaGcodeSuite::M706() {}
void __attribute__((weak)) PrusaGcodeSuite::M707() {}
void __attribute__((weak)) PrusaGcodeSuite::M708() {}
void __attribute__((weak)) PrusaGcodeSuite::M709() {}
void __attribute__((weak)) PrusaGcodeSuite::M930() {}
void __attribute__((weak)) PrusaGcodeSuite::M931() {}
void __attribute__((weak)) PrusaGcodeSuite::M932() {}
void __attribute__((weak)) PrusaGcodeSuite::M999() {}
void __attribute__((weak)) PrusaGcodeSuite::M1587() {}
void __attribute__((weak)) PrusaGcodeSuite::M1600() {}
void __attribute__((weak)) PrusaGcodeSuite::M1700() {}
void __attribute__((weak)) PrusaGcodeSuite::M1701() {}
void __attribute__((weak)) PrusaGcodeSuite::G162() {}
