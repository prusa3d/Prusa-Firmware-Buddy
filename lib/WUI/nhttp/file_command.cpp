#include "file_command.h"
#include "handler.h"
#include "json_parser.h"

#include <cassert>
#include <cstring>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace nhttp::printer {

using namespace handler;
using handler::Step;
using http::Status;
using json::Event;
using json::Type;
using std::nullopt;
using std::string_view;

namespace {

    enum class Command {
        Start,
        Unknown,
    };

}

FileCommand::FileCommand(const char *fname, size_t content_length, bool can_keep_alive, bool json_errors)
    : content_length(content_length)
    , can_keep_alive(can_keep_alive)
    , json_errors(json_errors) {
    memset(buffer.data(), 0, buffer.size());
    strlcpy(filename, fname, sizeof filename);
}

handler::StatusPage FileCommand::process() {
    Command command = Command::Unknown;
    const auto parse_result = parse_command(reinterpret_cast<char *>(buffer.data()), buffer_used, [&](const Event &event) {
        if (event.depth != 1 || event.type != Type::String) {
            return;
        }
        if (event.key == "command") {
            if (event.value == "start") {
                command = Command::Start;
            }
        }
    });

    switch (parse_result) {
    case JsonParseResult::ErrMem:
        return StatusPage(Status::PayloadTooLarge, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Too many JSON tokens");
    case JsonParseResult::ErrReq:
        return StatusPage(Status::BadRequest, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Couldn't parse JSON");
    case JsonParseResult::Ok:
        break;
    }

    switch (command) {
    case Command::Unknown:
        // Any idea for better status than the very generic 400? 404?
        return StatusPage(Status::BadRequest, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Unknown file command");
    case Command::Start:
        switch (start()) {
        case StartResult::NotReady:
            return StatusPage(Status::Conflict, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Can't start print now");
        case StartResult::NotFound:
            return StatusPage(Status::NotFound, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Gcode doesn't exist");
        case StartResult::Started:
            return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        }
        [[fallthrough]];
    default:
        assert(0);
        return StatusPage(Status::InternalServerError, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Invalid command");
    }
}

void FileCommand::step(string_view input, bool terminated_by_client, uint8_t *, size_t, Step &out) {
    if (content_length > buffer.size()) {
        // Refuse early, without reading the body -> drop the connection too.
        out = Step { 0, 0, StatusPage(Status::PayloadTooLarge, StatusPage::CloseHandling::ErrorClose, json_errors) };
        return;
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
            out = Step { to_read, 0, StatusPage(Status::BadRequest, StatusPage::CloseHandling::ErrorClose, json_errors, nullopt, "Truncated request") };
            return;
        } else {
            out = Step { to_read, 0, Continue() };
            return;
        }
    }

    out = Step { to_read, 0, process() };
    return;
}

} // namespace nhttp::printer
