#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

namespace con {

using CommandId = uint32_t;

struct UnknownCommand {};
struct BrokenCommand {};
struct Gcode {
    // TODO: Something goes in here.
};
struct SendInfo {};
struct SendJobInfo {
    uint16_t job_id;
};

using CommandData = std::variant<UnknownCommand, BrokenCommand, Gcode, SendInfo, SendJobInfo>;

struct Command {
    CommandId id;
    CommandData command_data;
    // Note: Might be a "Broken" command or something like that. In both cases.
    static Command gcode_command(CommandId id, const std::string_view &body);
    static Command parse_json_command(CommandId id, const std::string_view &body);
};

}
