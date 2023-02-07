#pragma once

#include <common/shared_buffer.hpp>

#include <cstdint>
#include <string_view>
#include <variant>

namespace connect_client {

using CommandId = uint32_t;

struct UnknownCommand {};
struct BrokenCommand {
    const char *reason = nullptr;
};
struct ProcessingOtherCommand {};
struct ProcessingThisCommand {};
struct GcodeTooLarge {};
struct Gcode {
    // Stored without the \0 at the end
    SharedBorrow gcode;
    size_t size;
};
struct SendInfo {};
struct SendJobInfo {
    uint16_t job_id;
};
struct SendFileInfo {
    SharedPath path;
};
struct SendTransferInfo {};
struct PausePrint {};
struct ResumePrint {};
struct StopPrint {};
struct StartPrint {
    SharedPath path;
};
struct SetPrinterReady {};
struct CancelPrinterReady {};
struct StartConnectDownload {
    // The hash itself is max 28 chars
    static constexpr size_t HASH_BUFF = 29;
    SharedPath path;
    uint64_t team;
    // In the original string form
    char hash[HASH_BUFF];
};
struct DeleteFile {
    SharedPath path;
};
struct DeleteFolder {
    SharedPath path;
};
struct CreateFolder {
    SharedPath path;
};
struct StopTransfer {};

using CommandData = std::variant<UnknownCommand, BrokenCommand, GcodeTooLarge, ProcessingOtherCommand, ProcessingThisCommand, Gcode, SendInfo, SendJobInfo, SendFileInfo, SendTransferInfo, PausePrint, ResumePrint, StopPrint, StartPrint, SetPrinterReady, CancelPrinterReady, StartConnectDownload, DeleteFile, DeleteFolder, CreateFolder, StopTransfer>;

struct Command {
    CommandId id;
    CommandData command_data;
    // Note: Might be a "Broken" command or something like that. In both cases.
    static Command gcode_command(CommandId id, const std::string_view &body, SharedBuffer::Borrow buff);
    // The buffer is either used and embedded inside the returned command or destroyed, releasing the ownership.
    static Command parse_json_command(CommandId id, const std::string_view &body, SharedBuffer::Borrow buff);
};

}
