#include "command.hpp"

#include <search_json.h>

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
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    jsmn_init(&parser);

    const auto parse_result = jsmn_parse(&parser, body.data(), body.size(), tokens, sizeof tokens / sizeof *tokens);

    if (parse_result <= 0 || tokens[0].type != JSMN_OBJECT) {
        // TODO: Be more specific about what went wrong.
        return Command {
            id,
            CommandType::Broken,
        };
    }

    CommandType command_type = CommandType::Unknown;

    // FIXME: Right now, our json::search doesn't know how to handle
    // sub-structures (arrays, objects). It returns false once they are
    // encountered. For now we only handle param-less commands, so if we hope
    // the command itself comes first, we are OK just ignoring the false
    // returned here for now.
    json::search(body.data(), tokens, parse_result, [&](const string_view &key, const string_view &value) {
        if (key == "command") {
            if (value == "SEND_INFO") {
                command_type = CommandType::SendInfo;
            }
        }
    });

    // Good. We have a "parsed" json.
    return Command {
        id,
        command_type,
    };
}

}
