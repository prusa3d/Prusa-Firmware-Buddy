#include "command.hpp"

#include <search_json.h>

using json::Event;
using json::Type;
using std::string_view;

namespace con {

namespace {

    // We are not completely sure what a token is in the notion of jsmn, besides we
    // may need a bit more because it's allowed to put more data in there. It's on
    // stack, so likely fine to overshoot a bit.
    const constexpr size_t MAX_TOKENS = 60;

}

Command Command::gcode_command(CommandId id, const string_view &body) {
    // TODO: We need to stuff the body somewhere. And we need to have an owned
    // variant, since the command wil live longer than the caller. Some kind of
    // std::string? But that's dynamic allocation :-(.
    return Command {
        id,
        CommandType::Gcode,
    };
}

Command Command::parse_json_command(CommandId id, const string_view &body) {
    jsmntok_t tokens[MAX_TOKENS];

    int parse_result;

    {
        jsmn_parser parser;
        jsmn_init(&parser);

        parse_result = jsmn_parse(&parser, body.data(), body.size(), tokens, sizeof tokens / sizeof *tokens);
    } // Free the parser

    CommandType command_type = CommandType::Unknown;

    // Error from jsmn_parse will lead to -1 -> converted to 0, refused by json::search as Broken.
    const bool success = json::search(body.data(), tokens, std::max(parse_result, 0), [&](const Event &event) {
        if (event.depth != 1 || event.type != Type::String) {
            return;
        }
        if (event.key == "command") {
            if (event.value == "SEND_INFO") {
                command_type = CommandType::SendInfo;
            }
        }
    });

    if (!success) {
        command_type = CommandType::Broken;
    }

    // Good. We have a "parsed" json.
    return Command {
        id,
        command_type,
    };
}

}
