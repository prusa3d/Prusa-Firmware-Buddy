/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "feature/prusa/MMU2/mmu2_mk4.h"
#include "gcode_info.hpp"
#include <algorithm>
#include <cctype>
#include <utils/string_builder.hpp>
#include <vector>

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M862.6: Check supported features <a href="https://reprap.org/wiki/G-code#M862.6:_Firmware_features">M862.6: Firmware features</a>
 *
 *#### Usage
 *
 *    M [ Q | P "<string>" ]
 *
 *#### Parameters
 *
 * - `Q` - Print current supported features
 * - `P "<string>"` - Check for feature
 */

void PrusaGcodeSuite::M862_6() {
    std::vector<std::string_view> supported_features;
    for (auto &feature : GCodeInfo::supported_features) {
        supported_features.push_back(feature);
    }
    #if ENABLED(PRUSA_MMU2)
    if (MMU2::mmu2.Enabled()) {
        supported_features.push_back("MMU3");
    }
    #endif
    if (parser.boolval('Q')) {
        for (auto &feature : supported_features) {
            SERIAL_ECHO_START();
            SERIAL_ECHO("  M862.6 P\"");
            SERIAL_ECHO(feature.data());
            SERIAL_ECHOLN("\"");
        }
    }

    if (parser.seenval('P')) {
        const char *arg = parser.string_arg;
        while (*arg == ' ' || *arg == '\"') {
            arg++;
        }

        const char *arg_end = arg;
        while (*arg_end && *arg_end != '\"') {
            arg_end++;
        }

        const auto str_arg = std::string_view(arg, arg_end - arg);
        ArrayStringBuilder<80> sb;
        sb.append_printf("\"");
        sb.append_std_string_view(str_arg);
        sb.append_printf("\"");
        SERIAL_ECHO(sb.str());
        if (!std::count(supported_features.begin(), supported_features.end(), str_arg)) {
            SERIAL_ECHO(" NOT");
        }
        SERIAL_ECHOLN(" supported.");
    }
}

/** @}*/

#endif
