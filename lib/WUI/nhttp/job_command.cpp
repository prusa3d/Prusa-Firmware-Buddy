#include "job_command.h"
#include "handler.h"

#define JSMN_HEADER
#include <jsmn.h>

#include <cassert>
#include <cstring>

namespace nhttp::printer {

using namespace handler;
using std::string_view;

namespace {

    /*
 * We are not completely sure what a token is in the notion of jsmn, besides we
 * may need a bit more because it's allowed to put more data in there. It's on
 * stack, so likely fine to overshoot a bit.
 */
    const constexpr size_t MAX_TOKENS = 30;

    template <class Callback>
    bool search_json(const char *input, jsmntok_t *tokens, size_t cnt, Callback &&callback) {
        if (cnt == 0) {
            return false;
        }

        if (tokens[0].type != JSMN_OBJECT) {
            return false;
        }

        // #0 is the top-level object.
        for (size_t i = 1; i < cnt; i++) {
            if (tokens[i].type == JSMN_STRING) {
                /*
                 * FIXME: jsmn doesn't decode the strings. We simply hope they
                 * don't contain any escape sequences.
                 */
                string_view key(input + tokens[i].start, tokens[i].end - tokens[i].start);
                // Parsing made sure there's another one.
                auto &val = tokens[i + 1];
                switch (val.type) {
                case JSMN_STRING: {
                    string_view value(input + val.start, val.end - val.start);
                    callback(key, value);
                    // Fall through to primitive
                }
                case JSMN_PRIMITIVE:
                    i++;
                    break;
                case JSMN_ARRAY:
                case JSMN_OBJECT:
                    /*
                     * FIXME: These are not yet implemented. We need to deal somehow with all the tokens. Options:
                     * * Use the parent links. Nevertheless, it seems enabling
                     *   it for jsmn confuses it and it simply returs fully bogus
                     *   results.
                     * * Understand the structure and traverse it. A lot of work to do.
                     */
                default:
                    return false;
                }
            } else {
                // Non-string key...
                return false;
            }
        }

        return true;
    }

}

JobCommand::JobCommand(size_t content_length, bool can_keep_alive)
    : content_length(content_length)
    , can_keep_alive(can_keep_alive) {
    memset(buffer.data(), 0, buffer.size());
}

Step JobCommand::step(std::string_view input, bool terminated_by_client, uint8_t *, size_t) {
    if (content_length > buffer.size()) {
        // Refuse early, without reading the body -> drop the connection too.
        return Step { 0, 0, StatusPage(Status::PayloadTooLarge, false) };
    }

    const size_t rest = content_length - buffer_used;
    const size_t to_read = std::min(input.size(), rest);

    // FIXME: Actually, we could be sent a chunked-encoded post and this would
    // not work properly. We should be decoding that :-(. At that point we
    // should also accept it if there's no up-front content length.
    memcpy(buffer.data() + buffer_used, input.data(), to_read);
    buffer_used += to_read;

    if (content_length > buffer_used) {
        // Still waiting for more data.
        if (terminated_by_client) {
            return Step { to_read, 0, StatusPage(Status::BadRequest, false, "Truncated request") };
        } else {
            return Step { to_read, 0, Continue() };
        }
    }

    return Step { to_read, 0, process() };
}

StatusPage JobCommand::process() {
    switch (parse_command()) {
    case Command::ErrMem:
        return StatusPage(Status::PayloadTooLarge, can_keep_alive, "Too many JSON tokens");
    case Command::ErrReq:
        return StatusPage(Status::BadRequest, can_keep_alive, "Couldn't parse JSON");
    case Command::ErrUnknownCommand:
        // Any idea for better status than the very generic 400? 404?
        return StatusPage(Status::BadRequest, can_keep_alive, "Unknown job command");
    case Command::Pause:
        if (pause()) {
            return StatusPage(Status::NoContent, can_keep_alive);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive);
        }
    case Command::Resume:
        if (resume()) {
            return StatusPage(Status::NoContent, can_keep_alive);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive);
        }
    case Command::PauseToggle:
        if (pause_toggle()) {
            return StatusPage(Status::NoContent, can_keep_alive);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive);
        }
    case Command::Stop:
        if (stop()) {
            return StatusPage(Status::NoContent, can_keep_alive);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive);
        }
    default:
        assert(0);
        return StatusPage(Status::InternalServerError, can_keep_alive, "Invalid command");
    }
}

JobCommand::Command JobCommand::parse_command() {
    /*
     * For technical reasons in its own function. This releases the used stack
     * before going to talk to marlin (which reportedly uses large stack too).
     */
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    jsmn_init(&parser);
    const char *strbuf = reinterpret_cast<const char *>(buffer.data());
    const auto parse_result = jsmn_parse(&parser, strbuf, buffer_used, tokens, sizeof tokens / sizeof *tokens);

    if (parse_result < 0) {
        if (parse_result == JSMN_ERROR_NOMEM) {
            // Too few tokens, give up.
            return Command::ErrMem;
        } else {
            // Something else is wrong...
            return Command::ErrReq;
        }
    } else {
        Command pause_command = Command::ErrUnknownCommand;
        Command top_command = Command::ErrUnknownCommand;

        const bool success = search_json(strbuf, tokens, parse_result, [&](string_view key, string_view value) {
            if (key == "command") {
                if (value == "cancel") {
                    top_command = Command::Stop;
                } else if (value == "pause") {
                    top_command = Command::Pause;
                }
            } else if (key == "action") {
                if (value == "pause") {
                    pause_command = Command::Pause;
                } else if (value == "resume") {
                    pause_command = Command::Resume;
                } else if (value == "toggle") {
                    pause_command = Command::PauseToggle;
                }
            }
        });

        if (success) {
            if (top_command == Command::Pause) {
                return pause_command;
            } else {
                return top_command;
            }
        } else {
            return Command::ErrReq;
        }
    }
}

}
