#pragma once

#include <cstdint>
#include <string_view>

namespace con {

using CommandId = uint32_t;

enum class CommandType {
    Unknown,
    // Something is wrong about the command. Possibly too large or malformed.
    Broken,
    Gcode,
    SendInfo,
};

struct Command {
    CommandId id;
    CommandType type;
    // Note: Might be a "Broken" command or something like that. In both cases.
    static Command gcode_command(CommandId id, const std::string_view &body);
    static Command parse_json_command(CommandId id, const std::string_view &body);
};

}
