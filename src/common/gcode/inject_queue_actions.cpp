/// @file
#include "inject_queue_actions.hpp"

#include <string_builder.hpp>

void GCodeLiteral::to_string(StringBuilder &sb) const {
    sb.append_printf(gcode, (double)parameter);
    release_assert(sb.is_ok());
}
