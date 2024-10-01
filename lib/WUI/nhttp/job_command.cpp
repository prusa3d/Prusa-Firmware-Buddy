#include "job_command.h"
#include "handler.h"
#include "json_parser.h"

#include <cassert>
#include <cstring>

namespace nhttp::printer {

using namespace handler;
using http::Status;
using json::Event;
using json::Type;
using std::nullopt;
using std::string_view;

namespace {

    enum class Command {
        ErrUnknownCommand,
        Stop,
        Pause,
        Resume,
        PauseToggle,
    };

}

JobCommand::JobCommand(size_t content_length, bool can_keep_alive, bool json_errors)
    : content_length(content_length)
    , can_keep_alive(can_keep_alive)
    , json_errors(json_errors) {
    memset(buffer.data(), 0, buffer.size());
}

void JobCommand::step(std::string_view input, bool terminated_by_client, uint8_t *, size_t, Step &out) {
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
}

StatusPage JobCommand::process() {
    Command pause_command = Command::ErrUnknownCommand;
    Command top_command = Command::ErrUnknownCommand;

    const auto parse_result = parse_command(reinterpret_cast<char *>(buffer.data()), buffer_used, [&](const Event &event) {
        if (event.depth != 1 || event.type != Type::String) {
            return;
        }
        const auto &key = event.key.value();
        const auto &value = event.value.value();
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

    switch (parse_result) {
    case JsonParseResult::ErrMem:
        return StatusPage(Status::PayloadTooLarge, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Too many JSON tokens");
    case JsonParseResult::ErrReq:
        return StatusPage(Status::BadRequest, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Couldn't parse JSON");
    case JsonParseResult::Ok:
        break;
    }

    if (top_command == Command::Pause) {
        top_command = pause_command;
    }

    switch (top_command) {
    case Command::ErrUnknownCommand:
        // Any idea for better status than the very generic 400? 404?
        return StatusPage(Status::BadRequest, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, "Unknown job command");
    case Command::Pause:
        if (pause()) {
            return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        }
    case Command::Resume:
        if (resume()) {
            return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        }
    case Command::PauseToggle:
        if (pause_toggle()) {
            return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        }
    case Command::Stop:
        if (stop()) {
            return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        } else {
            return StatusPage(Status::Conflict, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
        }
    default:
        assert(0);
        return StatusPage(Status::InternalServerError, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Invalid command");
    }
}

} // namespace nhttp::printer
