#pragma once

#include "buffer.hpp"

#include <cstdint>
#include <string_view>
#include <variant>

namespace connect_client {

using CommandId = uint32_t;

struct UnknownCommand {};
struct BrokenCommand {};
struct ProcessingOtherCommand {};
struct Gcode {
    // TODO: Something goes in here.
};
struct SendInfo {};
struct SendJobInfo {
    uint16_t job_id;
};
struct SendFileInfo {
    SharedPath path;
};
struct PausePrint {};
struct ResumePrint {};
struct StopPrint {};
struct StartPrint {
    SharedPath path;
};
struct SetPrinterReady {};
struct CancelPrinterReady {};

using CommandData = std::variant<UnknownCommand, BrokenCommand, ProcessingOtherCommand, Gcode, SendInfo, SendJobInfo, SendFileInfo, PausePrint, ResumePrint, StopPrint, StartPrint, SetPrinterReady, CancelPrinterReady>;

struct Command {
    CommandId id;
    CommandData command_data;
    // Note: Might be a "Broken" command or something like that. In both cases.
    static Command gcode_command(CommandId id, const std::string_view &body);
    // The buffer is either used and embedded inside the returned command or destroyed, releasing the ownership.
    static Command parse_json_command(CommandId id, const std::string_view &body, SharedBuffer::Borrow buff);
};

}
