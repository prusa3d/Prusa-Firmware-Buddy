#include "command.hpp"

#include <search_json.h>
#include <json_encode.h>

#include <cstdlib>
#include <charconv>

using json::Event;
using json::Type;
using std::array;
using std::errc;
using std::from_chars;
using std::get_if;
using std::make_shared;
using std::min;
using std::move;
using std::nullopt;
using std::optional;
using std::string_view;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace connect_client {

namespace {

    // We are not completely sure what a token is in the notion of jsmn, besides we
    // may need a bit more because it's allowed to put more data in there. It's on
    // stack, so likely fine to overshoot a bit.
    const constexpr size_t MAX_TOKENS = 60;

    template <class R>
    optional<R> convert_int(const Event &event) {
        if (R result; from_chars(event.value->begin(), event.value->end(), result).ec == errc {}) {
            return result;
        } else {
            return nullopt;
        }
    }

    template <size_t S>
    bool decode_hex(const Event &event, array<uint8_t, S> &dest) {
        if (event.value->size() != 2 * S) {
            return false;
        }
        const char *input = event.value->begin();
        for (size_t i = 0; i < dest.size(); i++) {
            if (from_chars(input + 2 * i, input + 2 * (i + 1), dest[i], 16).ec != errc {}) {
                return false;
            }
        }
        return true;
    }

    enum HasArg : uint32_t {
        ArgJobId = 1 << 0,
        ArgPath = 1 << 1,
        ArgToken = 1 << 2,
        ArgKey = 1 << 3,
        ArgIv = 1 << 4,
        ArgOrigSize = 1 << 5,
    };

    constexpr uint32_t NO_ARGS = 0;
    // Encrypted download can also process a port, but that one is optional, so not listed here.
    constexpr uint32_t ARGS_ENC_DOWN = ArgPath | ArgKey | ArgIv | ArgOrigSize;
} // namespace

Command Command::gcode_command(CommandId id, const string_view &body, SharedBuffer::Borrow buff) {
    if (body.size() > buff.size()) {
        return Command {
            id,
            GcodeTooLarge {},
        };
    }

    memcpy(buff.data(), body.data(), body.size());
    return Command {
        id,
        Gcode {
            make_shared<SharedBuffer::Borrow>(move(buff)),
            body.size(),
        },
    };
}

Command Command::parse_json_command(CommandId id, char *body, size_t body_size, SharedBuffer::Borrow buff) {
    jsmntok_t tokens[MAX_TOKENS];

    int parse_result;

    {
        jsmn_parser parser;
        jsmn_init(&parser);

        parse_result = jsmn_parse(&parser, body, body_size, tokens, sizeof tokens / sizeof *tokens);
    } // Free the parser

    CommandData data = UnknownCommand {};

    bool in_kwargs = false;
    bool buffer_available = true;
    uint32_t expected_args = 0;
    uint32_t seen_args = 0;

    // Error from jsmn_parse will lead to -1 -> converted to 0, refused by json::search as Broken.
    const bool success = json::search(body, tokens, std::max(parse_result, 0), [&](const Event &event) {
        auto is_arg = [&](const string_view name, Type type) -> bool {
            return event.depth == 2 && in_kwargs && event.type == type && event.key == name;
        };
        if (event.depth == 1 && event.type == Type::String && event.key == "command") {
            // Will fill in all the insides later on, if needed
#define T(NAME, TYPE, EXP)     \
    if (event.value == NAME) { \
        data = TYPE {};        \
        expected_args = EXP;   \
    } else
            T("SEND_INFO", SendInfo, NO_ARGS)
            T("SEND_JOB_INFO", SendJobInfo, ArgJobId)
            T("SEND_FILE_INFO", SendFileInfo, ArgPath)
            T("SEND_TRANSFER_INFO", SendTransferInfo, NO_ARGS)
            T("PAUSE_PRINT", PausePrint, NO_ARGS)
            T("STOP_PRINT", StopPrint, NO_ARGS)
            T("RESUME_PRINT", ResumePrint, NO_ARGS)
            T("START_PRINT", StartPrint, ArgPath)
            T("SET_PRINTER_READY", SetPrinterReady, NO_ARGS)
            T("CANCEL_PRINTER_READY", CancelPrinterReady, NO_ARGS)
            T("DELETE_FILE", DeleteFile, ArgPath)
            T("DELETE_FOLDER", DeleteFolder, ArgPath)
            T("CREATE_FOLDER", CreateFolder, ArgPath)
            T("STOP_TRANSFER", StopTransfer, NO_ARGS)
            T("SET_TOKEN", SetToken, ArgToken)
            T("START_ENCRYPTED_DOWNLOAD", StartEncryptedDownload, ARGS_ENC_DOWN) { // else is part of the previous T
                return;
            }
        }

        // We use macros to get rid of repetitive bits. There probably is some
        // kind of solution with templated lambda functions taking a
        // pointer-to-member parameters ‒ but that would be hard to figure out
        // and arcane („If it was hard to write, it should be hard to read too“
        // kind).
#define INT_ARG(TYPE, FIELD_TYPE, FIELD, MARK)                                \
    if (auto *cmd = get_if<TYPE>(&data); cmd != nullptr) {                    \
        if (auto value = convert_int<FIELD_TYPE>(event); value.has_value()) { \
            cmd->FIELD = *value;                                              \
            seen_args |= MARK;                                                \
        }                                                                     \
    }
#define PATH_ARG(TYPE)                                                                                           \
    if (auto *cmd = get_if<TYPE>(&data); cmd != nullptr && buffer_available) {                                   \
        const size_t len = min(event.value->size() + 1, buff.size());                                            \
        strlcpy(reinterpret_cast<char *>(buff.data()), event.value->data(), len);                                \
        if (json_unescape_bytes(reinterpret_cast<char *>(buff.data()), reinterpret_cast<char *>(buff.data()))) { \
            cmd->path = SharedPath(move(buff));                                                                  \
            buffer_available = false;                                                                            \
            seen_args |= ArgPath;                                                                                \
        }                                                                                                        \
    }
#define HEX_ARG(FIELD, MARK)                                                 \
    if (auto *cmd = get_if<StartEncryptedDownload>(&data); cmd != nullptr) { \
        if (decode_hex(event, cmd->FIELD)) {                                 \
            seen_args |= MARK;                                               \
        }                                                                    \
    }

        if (event.depth == 1 && event.type == Type::Object && event.key == "kwargs") {
            in_kwargs = true;
        } else if (event.depth == 1 && event.type == Type::Pop) {
            in_kwargs = false;
        } else if (is_arg("job_id", Type::Primitive)) {
            INT_ARG(SendJobInfo, uint16_t, job_id, ArgJobId)
        } else if (is_arg("path", Type::String) || is_arg("path_sfn", Type::String)) {
            PATH_ARG(SendFileInfo)
            PATH_ARG(StartPrint)
            PATH_ARG(DeleteFile)
            PATH_ARG(DeleteFolder)
            PATH_ARG(CreateFolder)
            PATH_ARG(StartEncryptedDownload)
        } else if (is_arg("token", Type::String)) {
            if (auto *cmd = get_if<SetToken>(&data); cmd != nullptr && buffer_available) {
                const size_t len = min(event.value->size() + 1, buff.size());
                if (len - 1 <= Printer::Config::CONNECT_TOKEN_LEN) {
                    strlcpy(reinterpret_cast<char *>(buff.data()), event.value->data(), len);
                    cmd->token = std::make_shared<SharedBuffer::Borrow>(move(buff));
                    seen_args |= ArgToken;
                    buffer_available = false;
                } else {
                    data = BrokenCommand { "Token too long" };
                }
            }
        } else if (is_arg("port", Type::Primitive)) {
            INT_ARG(StartEncryptedDownload, uint16_t, port, 0)
        } else if (is_arg("orig_size", Type::Primitive)) {
            INT_ARG(StartEncryptedDownload, uint64_t, orig_size, ArgOrigSize)
        } else if (is_arg("key", Type::String)) {
            HEX_ARG(key, ArgKey)
        } else if (is_arg("iv", Type::String)) {
            HEX_ARG(iv, ArgIv)
        }
    });

    if (expected_args & ~seen_args) {
        data = BrokenCommand { "Missing or broken parameters" };
    }

    if (!success) {
        data = BrokenCommand { "Error parsing JSON" };
    }

    // Good. We have a "parsed" json.
    return Command {
        id,
        data,
    };
}

} // namespace connect_client
