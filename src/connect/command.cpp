#include "command.hpp"

#include <search_json.h>
#include <codepage/437.hpp>

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

    // Allow gathering the data about a START_ENCRYPTED_DOWNLOAD command.
    //
    // Allows tracking which parts are already available.
    struct DownloadAccumulator {
        bool ok = true;
        StartEncryptedDownload::Block key;
        StartEncryptedDownload::Block iv;
        size_t orig_size;
        bool has_key = false;
        bool has_iv = false;
        bool has_size = false;
        bool validate() const {
            return ok && has_key && has_iv && has_size;
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
    };
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
    optional<uint16_t> job_id = nullopt;
    optional<uint16_t> port = nullopt;

    DownloadAccumulator download_acc;
    bool has_path = false;

    // Error from jsmn_parse will lead to -1 -> converted to 0, refused by json::search as Broken.
    const bool success = json::search(body, tokens, std::max(parse_result, 0), [&](const Event &event) {
        auto is_arg = [&](const string_view name, Type type) -> bool {
            return event.depth == 2 && in_kwargs && event.type == type && event.key == name;
        };
        if (event.depth == 1 && event.type == Type::String && event.key == "command") {
            // Will fill in all the insides later on, if needed
#define T(NAME, TYPE)          \
    if (event.value == NAME) { \
        data = TYPE {};        \
    } else
            T("SEND_INFO", SendInfo)
            T("SEND_JOB_INFO", SendJobInfo)
            T("SEND_FILE_INFO", SendFileInfo)
            T("SEND_TRANSFER_INFO", SendTransferInfo)
            T("PAUSE_PRINT", PausePrint)
            T("STOP_PRINT", StopPrint)
            T("RESUME_PRINT", ResumePrint)
            T("START_PRINT", StartPrint)
            T("SET_PRINTER_READY", SetPrinterReady)
            T("CANCEL_PRINTER_READY", CancelPrinterReady)
            T("DELETE_FILE", DeleteFile)
            T("DELETE_FOLDER", DeleteFolder)
            T("CREATE_FOLDER", CreateFolder)
            T("STOP_TRANSFER", StopTransfer)
            if (event.value == "START_ENCRYPTED_DOWNLOAD") {
                data = StartEncryptedDownload {};
            } else {
                return;
            }
        }

        if (event.depth == 1 && event.type == Type::Object && event.key == "kwargs") {
            in_kwargs = true;
        } else if (event.depth == 1 && event.type == Type::Pop) {
            in_kwargs = false;
        } else if (is_arg("job_id", Type::Primitive)) {
            job_id = convert_int<uint16_t>(event);
        } else if (is_arg("path", Type::String) || is_arg("path_sfn", Type::String)) {
            const size_t len = min(event.value->size() + 1, buff.size());
            strlcpy(reinterpret_cast<char *>(buff.data()), event.value->data(), len);
            has_path = true;
        } else if (is_arg("port", Type::Primitive)) {
            port = convert_int<uint16_t>(event);
        } else if (is_arg("key", Type::String)) {
            download_acc.has_key = download_acc.decode_hex(event, download_acc.key);
        } else if (is_arg("iv", Type::String)) {
            download_acc.has_iv = download_acc.decode_hex(event, download_acc.iv);
        } else if (is_arg("orig_size", Type::Primitive)) {
            if (auto val = convert_int<uint64_t>(event); val.has_value()) {
                download_acc.orig_size = *val;
                download_acc.has_size = true;
            } else {
                download_acc.ok = false;
            }
        }
    });

    if (!success) {
        data = BrokenCommand { "Error parsing json" };
    }

    auto get_path = [&](SharedPath &path) -> void {
        if (has_path) {
            // Include the final \0 in the decoding too, so the new one gets auto-terminated by it.
            codepage::utf8_to_cp437(buff.data(), strlen(reinterpret_cast<const char *>(buff.data())) + 1);
            path = SharedPath(move(buff));
        } else {
            // Missing parameters
            data = BrokenCommand { "missing path" };
        }
    };

    if (auto *info = get_if<SendJobInfo>(&data); info != nullptr) {
        if (job_id.has_value()) {
            info->job_id = *job_id;
        } else {
            // Didn't find all the needed parameters.
            data = BrokenCommand { "missing parameters" };
        }
    } else if (auto *info = get_if<SendFileInfo>(&data); info != nullptr) {
        get_path(info->path);
    } else if (auto *start = get_if<StartPrint>(&data); start != nullptr) {
        get_path(start->path);
    } else if (auto *del_file = get_if<DeleteFile>(&data); del_file != nullptr) {
        get_path(del_file->path);
    } else if (auto *del_folder = get_if<DeleteFolder>(&data); del_folder != nullptr) {
        get_path(del_folder->path);
    } else if (auto *create_folder = get_if<CreateFolder>(&data); create_folder != nullptr) {
        get_path(create_folder->path);
    } else if (auto *download = get_if<StartEncryptedDownload>(&data); download != nullptr) {
        const bool ok = has_path && download_acc.validate();
        if (ok) {
            // XXX: We need some decoding here.
            //
            // However, the path is partially SFN (the dirname of it) and
            // partially LFN (the basename/filename part). Each one needs
            // different decoding, postponing this one until later, since we
            // are not yet agreed on the way we'll be doing decoding of the LFN
            // (or if at all, on our side).
            download->path = SharedPath(move(buff));
            download->key = download_acc.key;
            download->iv = download_acc.iv;
            download->orig_size = download_acc.orig_size;
            download->port = port;
        } else {
            // Missing parameters, conflicting parameters, etc..
            data = BrokenCommand { "missing or wrong parameters" };
        }
    }

    // Good. We have a "parsed" json.
    return Command {
        id,
        data,
    };
}

} // namespace connect_client
