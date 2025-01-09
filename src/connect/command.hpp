#pragma once

#include "printer.hpp"

#include <common/shared_buffer.hpp>
#include <option/has_side_leds.h>

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
// NOTE: if you are changing this, change also the one in printer.hpp,
//  it is at both places, otherwise it would create circular dependencies
using ToolMapping = std::array<std::array<uint8_t, EXTRUDERS>, EXTRUDERS>;
struct StartPrint {
    SharedPath path;
    std::optional<ToolMapping> tool_mapping;
};
struct SetPrinterReady {};
struct CancelPrinterReady {};
struct SetPrinterIdle {};
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
struct StartInlineDownload {
    uint64_t team_id;
    uint32_t orig_size;
    SharedPath path;
    // TODO: In fact, that hash is base64-encoded 16-byte hash. We _could_ save
    // some space by decoding/encoding as necessary.
    static constexpr size_t HASH_BUFF = 29;
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
struct SetToken {
    SharedBorrow token;
};
struct ResetPrinter {};
struct SendStateInfo {};
struct DialogAction {
    uint32_t dialog_id;
    Response response;
};

#define NOZZLE_NAMES(i)      \
    Nozzle##i##Diameter,     \
        Nozzle##i##HighFlow, \
        Nozzle##i##AntiAbrasive

enum class PropertyName {
    HostName,
#if XL_ENCLOSURE_SUPPORT()
    EnclosureEnabled,
    EnclosurePrintingFiltration,
    EnclosurePostPrint,
    EnclosurePostPrintFiltrationTime,
#endif
    NozzleDiameter,
    NozzleHighFlow,
    NozzleHardened,
#if PRINTER_IS_PRUSA_COREONE() || defined(UNITTESTS)
    // Note: for now we only want to support the chamber features on Core One.
    // Therefore option HAS_CHAMBER_API is NOT used yet.
    ChamberTargetTemp,
    ChamberFanPwmTarget,
    AddonPower, // not a very descriptive name, but the Connect team understands this property name as USB power output on the XBE
#endif
#if HAS_SIDE_LEDS() || defined(UNITTESTS)
    ChamberLedIntensity,
#endif
};

#undef NOZZLE_NAMES

struct SetValue {
    PropertyName name;
    // For names that relate to stuff we have more of… like nozzles.
    size_t idx;
    std::variant<bool, uint32_t, float, int8_t, SharedBorrow> value;
};
struct CancelObject {
    uint8_t id;
};
struct UncancelObject {
    uint8_t id;
};

using CommandData = std::variant<UnknownCommand, BrokenCommand, GcodeTooLarge, ProcessingOtherCommand, ProcessingThisCommand, Gcode, SendInfo, SendJobInfo, SendFileInfo, SendTransferInfo, PausePrint, ResumePrint, StopPrint, StartPrint, SetPrinterReady, CancelPrinterReady, SetPrinterIdle, StartEncryptedDownload, StartInlineDownload, DeleteFile, DeleteFolder, CreateFolder, StopTransfer, SetToken, ResetPrinter, SendStateInfo, DialogAction, SetValue, CancelObject, UncancelObject>;

struct Command {
    CommandId id;
    CommandData command_data;
    // Note: Might be a "Broken" command or something like that. In both cases.
    static Command gcode_command(CommandId id, const std::string_view &body, SharedBuffer::Borrow buff);
    // The buffer is either used and embedded inside the returned command or destroyed, releasing the ownership.
    static Command parse_json_command(CommandId id, char *body, size_t body_size, SharedBuffer::Borrow buff);
};

} // namespace connect_client
