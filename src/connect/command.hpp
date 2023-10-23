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
// This command actually implements both the START_CONNECT_DOWNLOAD and
// START_ENCRYPTED_DOWNLOAD. The reason is, for us, the commands are really
// similar and we reuse a lot of code for it.
struct StartEncryptedDownload {
    SharedPath path;
    // Port override.
    std::optional<uint16_t> port;

    static constexpr size_t BLOCK_SIZE = 16;
    using Block = std::array<uint8_t, BLOCK_SIZE>;
    Block key;
    // Legacy name (in the json too!) from times we used AES-CBC. With AES-CTR, it is used an nonce.
    Block iv;
    // Fatfs can't do bigger than 4GB files anyway, right?
    uint32_t orig_size;
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

using CommandData = std::variant<UnknownCommand, BrokenCommand, GcodeTooLarge, ProcessingOtherCommand, ProcessingThisCommand, Gcode, SendInfo, SendJobInfo, SendFileInfo, SendTransferInfo, PausePrint, ResumePrint, StopPrint, StartPrint, SetPrinterReady, CancelPrinterReady, StartEncryptedDownload, DeleteFile, DeleteFolder, CreateFolder, StopTransfer>;

struct Command {
    CommandId id;
    CommandData command_data;
    // Note: Might be a "Broken" command or something like that. In both cases.
    static Command gcode_command(CommandId id, const std::string_view &body, SharedBuffer::Borrow buff);
    // The buffer is either used and embedded inside the returned command or destroyed, releasing the ownership.
    static Command parse_json_command(CommandId id, char *body, size_t body_size, SharedBuffer::Borrow buff);
};

} // namespace connect_client
