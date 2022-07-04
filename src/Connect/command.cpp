#include "command.hpp"

#include <search_json.h>

#include <cstring>
#include <cstdlib>

using json::Event;
using json::Type;
using std::get_if;
using std::nullopt;
using std::optional;
using std::string_view;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

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
        Gcode {},
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

    CommandData data = UnknownCommand {};

    bool in_kwargs = false;
    optional<uint16_t> job_id = nullopt;

    // Error from jsmn_parse will lead to -1 -> converted to 0, refused by json::search as Broken.
    const bool success = json::search(body.data(), tokens, std::max(parse_result, 0), [&](const Event &event) {
        if (event.depth == 1 && event.type == Type::String && event.key == "command") {
            if (event.value == "SEND_INFO") {
                data = SendInfo {};
            } else if (event.value == "SEND_JOB_INFO") {
                // We'll set the job ID later, we may or may not have parset it yet.
                data = SendJobInfo {};
            }
            return;
        }

        if (event.depth == 1 && event.type == Type::Object && event.key == "kwargs") {
            in_kwargs = true;
        } else if (event.depth == 1 && event.type == Type::Pop) {
            in_kwargs = false;
        } else if (event.depth == 2 && in_kwargs && event.type == Type::Primitive && event.key == "job_id") {
            // Does anyone know of a way to convert size-bounded (not null-terminated) string to number in C/C++? :-(
            const size_t len = event.value->size();
            char id_str[len + 1];
            strlcpy(id_str, event.value->data(), len + 1);
            char *err = nullptr;
            job_id = strtoul(id_str, &err, 0);
            if (err && *err) {
                job_id = nullopt;
            }
        }
    });

    if (!success) {
        data = BrokenCommand {};
    }

    if (auto *info = get_if<SendJobInfo>(&data); info != nullptr) {
        if (job_id.has_value()) {
            info->job_id = *job_id;
        } else {
            // Didn't find all the needed parameters.
            data = BrokenCommand {};
        }
    }

    // Good. We have a "parsed" json.
    return Command {
        id,
        data,
    };
}

}
