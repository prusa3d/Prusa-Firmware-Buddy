#include "gcode_parser_test_common.hpp"

#include <gcode_parser.hpp>

std::string option_list(GCodeParser2 &p) {
    std::string result;
    for (int ch = 0; ch < 256; ch++) {
        if (p.has_option(static_cast<char>(ch))) {
            result.push_back(static_cast<char>(ch));
        }
    }
    return result;
}
