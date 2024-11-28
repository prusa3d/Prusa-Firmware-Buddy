#include "command.hpp"

#include <search_json.h>
#include <json_encode.h>

#include <general_response.hpp>
#include <module/prusa/tool_mapper.hpp>
#include <netif_settings.h>

#include <logging/log.hpp>

#include <cstdlib>
#include <charconv>
#include <limits>

using json::Event;
using json::Type;
using std::array;
using std::errc;
using std::get_if;
using std::make_shared;
using std::min;
using std::move;
using std::nullopt;
using std::optional;
using std::string_view;

LOG_COMPONENT_REF(connect);

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
    optional<R> convert_num(std::string_view str) {
        if (R result; from_chars_light(str.begin(), str.end(), result).ec == errc {}) {
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
            if (from_chars_light(input + 2 * i, input + 2 * (i + 1), dest[i], 16).ec != errc {}) {
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
        ArgDialogId = 1 << 6,
        ArgResponse = 1 << 7,
        ArgSetValue = 1 << 8,
        ArgId = 1 << 9,
        ArgTeamId = 1 << 10,
        ArgHash = 1 << 11,
    };

    constexpr uint32_t NO_ARGS = 0;
    // Encrypted download can also process a port, but that one is optional, so not listed here.
    constexpr uint32_t ARGS_ENC_DOWN = ArgPath | ArgKey | ArgIv | ArgOrigSize;
    constexpr uint32_t ARGS_DIALOG_ACTION = ArgDialogId | ArgResponse;
    constexpr uint32_t ARGS_INLINE_DOWN = ArgPath | ArgOrigSize | ArgTeamId | ArgHash;
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
    log_info(connect, "Received commad %.*s", body_size, body);
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

#if ENABLED(PRUSA_TOOL_MAPPING)
    bool in_tool_mapping = false;
    uint32_t tool_mapping_index_outer = 0;
    uint32_t tool_mapping_index_inner = 0;
#endif

    // Error from jsmn_parse will lead to -1 -> converted to 0, refused by json::search as Broken.
    const bool success = json::search(body, tokens, std::max(parse_result, 0), [&](const Event &event) {
        auto is_arg = [&](const string_view name, Type type) -> bool {
            return event.depth == 2 && in_kwargs && event.type == type && event.key == name;
        };
#if ENABLED(PRUSA_TOOL_MAPPING)
        auto is_tool_mapping_index = [&]() -> std::optional<uint32_t> {
            if (in_tool_mapping && event.type == Type::Array && event.depth == 3) {
                return convert_num<uint32_t>(event.key.value());
            }
            return std::nullopt;
        };
#endif
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
            T("SET_IDLE", SetPrinterIdle, NO_ARGS)
            T("DELETE_FILE", DeleteFile, ArgPath)
            T("DELETE_FOLDER", DeleteFolder, ArgPath)
            T("CREATE_FOLDER", CreateFolder, ArgPath)
            T("STOP_TRANSFER", StopTransfer, NO_ARGS)
            T("SET_TOKEN", SetToken, ArgToken)

            // There actually are two different commands to reset. The
            // distinction comes from MK3, where the printer and the raspberry
            // can be reset separately, while here the distinction makes no
            // sense. But due to some discussion what command _is_ being used
            // and what command _should_ be used by the server, we simply
            // accept both variants and do the same.
            T("RESET_PRINTER", ResetPrinter, NO_ARGS)
            T("RESET", ResetPrinter, NO_ARGS)
            T("SEND_STATE_INFO", SendStateInfo, NO_ARGS)
            T("DIALOG_ACTION", DialogAction, ARGS_DIALOG_ACTION)
            T("SET_VALUE", SetValue, ArgSetValue)
            T("CANCEL_OBJECT", CancelObject, ArgId)
            T("UNCANCEL_OBJECT", UncancelObject, ArgId)
            T("START_INLINE_DOWNLOAD", StartInlineDownload, ARGS_INLINE_DOWN)
            // For technical reasons, Connect wants to have only one
            // (START_CONNECT_DOWNLOAD) command and let the printer figure if
            // it wants to do that through the websocket or "around". We do it
            // inline the websocket always, and for some time it's unclear
            // which command will be coming from which Connect instance ->
            // accept both, handle the same way.
            T("START_CONNECT_DOWNLOAD", StartInlineDownload, ARGS_INLINE_DOWN)
            T("START_ENCRYPTED_DOWNLOAD", StartEncryptedDownload, ARGS_ENC_DOWN) { // else is part of the previous T
                return;
            }
        }

        // We use macros to get rid of repetitive bits. There probably is some
        // kind of solution with templated lambda functions taking a
        // pointer-to-member parameters ‒ but that would be hard to figure out
        // and arcane („If it was hard to write, it should be hard to read too“
        // kind).
#define INT_ARG(TYPE, FIELD_TYPE, FIELD, MARK)                                              \
    if (auto *cmd = get_if<TYPE>(&data); cmd != nullptr) {                                  \
        if (auto value = convert_num<FIELD_TYPE>(event.value.value()); value.has_value()) { \
            cmd->FIELD = *value;                                                            \
            seen_args |= MARK;                                                              \
        }                                                                                   \
    }
#define PATH_ARG(TYPE)                                                            \
    if (auto *cmd = get_if<TYPE>(&data); cmd != nullptr && buffer_available) {    \
        const size_t len = min(event.value->size() + 1, buff.size());             \
        strlcpy(reinterpret_cast<char *>(buff.data()), event.value->data(), len); \
        cmd->path = SharedPath(move(buff));                                       \
        buffer_available = false;                                                 \
        seen_args |= ArgPath;                                                     \
    }
#define HEX_ARG(FIELD, MARK)                                                 \
    if (auto *cmd = get_if<StartEncryptedDownload>(&data); cmd != nullptr) { \
        if (decode_hex(event, cmd->FIELD)) {                                 \
            seen_args |= MARK;                                               \
        }                                                                    \
    }

#define SET_VALUE_ARG(name_, type)                              \
    if (auto *cmd = get_if<SetValue>(&data); cmd != nullptr) {  \
        seen_args |= ArgSetValue;                               \
        cmd->name = name_;                                      \
        auto value = convert_num<type>(event.value.value());    \
        if (value.has_value()) {                                \
            cmd->value = value.value();                         \
        } else {                                                \
            data = BrokenCommand { "Invalid " #type " value" }; \
        }                                                       \
    }

        auto set_value_bool_arg = [&](PropertyName name, size_t idx = 0) {
            if (auto *cmd = get_if<SetValue>(&data); cmd != nullptr) {
                seen_args |= ArgSetValue;
                cmd->name = name;
                cmd->idx = idx;
                if (event.value->compare("true") == 0) {
                    cmd->value = true;
                } else if (event.value->compare("false") == 0) {
                    cmd->value = false;
                } else {
                    data = BrokenCommand { "Invalid bool value" };
                }
            }
        };

        auto set_value_float_arg = [&](PropertyName name, size_t idx = 0) {
            if (auto *cmd = get_if<SetValue>(&data); cmd != nullptr) {
                seen_args |= ArgSetValue;
                cmd->name = name;
                cmd->idx = idx;
                auto value = convert_num<float>(event.value.value());
                if (value.has_value()) {
                    cmd->value = value.value();
                } else {
                    data = BrokenCommand { "Invalid float value" };
                }
            }
        };

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
            PATH_ARG(StartInlineDownload)
#if ENABLED(PRUSA_TOOL_MAPPING)
        } else if (is_arg("tool_mapping", Type::Object)) {
            in_tool_mapping = true;
            if (auto *cmd = get_if<StartPrint>(&data); cmd != nullptr) {
                cmd->tool_mapping.emplace();
                for (auto &ext : cmd->tool_mapping.value()) {
                    for (auto &join : ext) {
                        join = ToolMapper::NO_TOOL_MAPPED;
                    }
                }
            }
        } else if (in_tool_mapping && event.key->compare("tool_mapping") == 0 && event.type == Type::Pop && event.depth == 2) {
            in_tool_mapping = false;
        } else if (auto index = is_tool_mapping_index(); index.has_value()) {
            tool_mapping_index_inner = 0;
            // NOTE: Internally tools are numbered from 0, externally from 1.
            tool_mapping_index_outer = index.value() - 1;
        } else if (in_tool_mapping && event.type == Type::Primitive && event.depth == 4) {
            if (auto *cmd = get_if<StartPrint>(&data); cmd != nullptr) {
                if (tool_mapping_index_outer < cmd->tool_mapping.value().size()
                    && tool_mapping_index_inner < cmd->tool_mapping.value()[tool_mapping_index_outer].size()) {
                    auto tool = convert_num<uint32_t>(event.value.value());
                    if (tool.has_value()) {
                        cmd->tool_mapping.value()[tool_mapping_index_outer][tool_mapping_index_inner++] = tool.value() - 1;
                    } else {
                        data = BrokenCommand { "Invalid tool index" };
                    }
                } else {
                    data = BrokenCommand { "Tool index out of range" };
                }
            }
#endif
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
        } else if (is_arg("button", Type::String)) {
            if (auto *cmd = get_if<DialogAction>(&data); cmd != nullptr) {
                cmd->response = from_str(event.value.value());
                seen_args |= ArgResponse;
                if (cmd->response == Response::_none) {
                    data = BrokenCommand { "Invalid button" };
                }
            }

        } else if (is_arg("hostname", Type::String)) {
            if (auto *cmd = get_if<SetValue>(&data); cmd != nullptr && buffer_available) {
                const size_t len = min(event.value->size() + 1, buff.size());
                if (len - 1 <= HOSTNAME_LEN) {
                    seen_args |= ArgSetValue;
                    cmd->name = PropertyName::HostName;
                    strlcpy(reinterpret_cast<char *>(buff.data()), event.value->data(), len);
                    cmd->value = std::make_shared<SharedBuffer::Borrow>(move(buff));
                    buffer_available = false;
                } else {
                    data = BrokenCommand { "Hostname too long." };
                }
            }
#if XL_ENCLOSURE_SUPPORT()
        } else if (is_arg("enclosure_enabled", Type::Primitive)) {
            set_value_bool_arg(PropertyName::EnclosureEnabled);
        } else if (is_arg("enclosure_printing_filtration", Type::Primitive)) {
            set_value_bool_arg(PropertyName::EnclosurePrintingFiltration);
        } else if (is_arg("enclosure_postprint", Type::Primitive)) {
            set_value_bool_arg(PropertyName::EnclosurePostPrint);
        } else if (is_arg("enclosure_postprint_filtration_time", Type::Primitive)) {
            if (auto *cmd = get_if<SetValue>(&data); cmd != nullptr) {
                seen_args |= ArgSetValue;
                cmd->name = PropertyName::EnclosurePostPrintFiltrationTime;
                auto time = convert_num<uint32_t>(event.value.value());
                if (time.has_value()) {
                    cmd->value = time.value();
                } else {
                    data = BrokenCommand { "Invalid int value" };
                }
            }
#endif
#define NOZZLE_PARAMS(num)                                                \
    }                                                                     \
    else if (is_arg("tools." #num ".high_flow", Type::Primitive)) {       \
        set_value_bool_arg(PropertyName::NozzleHighFlow, num - 1);        \
    }                                                                     \
    else if (is_arg("tools." #num ".hardened", Type::Primitive)) {        \
        set_value_bool_arg(PropertyName::NozzleHardened, num - 1);        \
    }                                                                     \
    else if (is_arg("tools." #num ".nozzle_diameter", Type::Primitive)) { \
        set_value_float_arg(PropertyName::NozzleDiameter, num - 1);

            NOZZLE_PARAMS(1)
#if HAS_TOOLCHANGER() || UNITTESTS
            NOZZLE_PARAMS(2)
            NOZZLE_PARAMS(3)
            NOZZLE_PARAMS(4)
            NOZZLE_PARAMS(5)
#endif
#undef NOZZLE_PARAMS
        } else if (is_arg("port", Type::Primitive)) {
            INT_ARG(StartEncryptedDownload, uint16_t, port, 0)
        } else if (is_arg("orig_size", Type::Primitive)) {
            INT_ARG(StartEncryptedDownload, uint32_t, orig_size, ArgOrigSize)
            INT_ARG(StartInlineDownload, uint32_t, orig_size, ArgOrigSize)
        } else if (is_arg("team_id", Type::Primitive)) {
            INT_ARG(StartInlineDownload, uint64_t, team_id, ArgTeamId)
        } else if (is_arg("hash", Type::String)) {
            if (auto *cmd = get_if<StartInlineDownload>(&data); cmd != nullptr) {
                const size_t len = min(event.value->size() + 1, sizeof cmd->hash);
                strlcpy(cmd->hash, event.value->data(), len);
                seen_args |= ArgHash;
            }
        } else if (is_arg("key", Type::String)) {
            HEX_ARG(key, ArgKey)
        } else if (is_arg("iv", Type::String)) {
            HEX_ARG(iv, ArgIv)
        } else if (is_arg("dialog_id", Type::Primitive)) {
            INT_ARG(DialogAction, uint32_t, dialog_id, ArgDialogId)
        } else if (is_arg("id", Type::Primitive)) {
            INT_ARG(CancelObject, uint8_t, id, ArgId)
            INT_ARG(UncancelObject, uint8_t, id, ArgId)
#if PRINTER_IS_PRUSA_COREONE()
        } else if (is_arg("chamber.target_temp", Type::Primitive)) {
            SET_VALUE_ARG(PropertyName::ChamberTargetTemp, uint32_t);
#endif
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
} // namespace connect_client

} // namespace connect_client
