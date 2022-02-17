#include "prusa_link_api.h"
#include "../nhttp/file_info.h"
#include "../nhttp/headers.h"
#include "../nhttp/gcode_upload.h"
#include "../nhttp/job_command.h"
#include "../wui_REST_api.h"

#include <cstring>
#include <cstdio>
#include <cerrno>

namespace nhttp::link_content {

using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;
using nhttp::printer::FileInfo;
using nhttp::printer::GcodeUpload;
using nhttp::printer::JobCommand;

namespace {

#define GET_WRAPPER(NAME)                                        \
    size_t handler_##NAME(uint8_t *buffer, size_t buffer_size) { \
        char *b = reinterpret_cast<char *>(buffer);              \
        NAME(b, buffer_size);                                    \
        return strlen(b);                                        \
    }
    GET_WRAPPER(get_printer);
    GET_WRAPPER(get_version);
    GET_WRAPPER(get_job);
#undef GET_WRAPPER

    optional<string_view> remove_prefix(string_view input, string_view prefix) {
        if (input.size() < prefix.size() || input.substr(0, prefix.size()) != prefix) {
            return nullopt;
        }

        return input.substr(prefix.size());
    }

}

optional<ConnectionState> PrusaLinkApi::accept(const RequestParser &parser) const {
    const string_view uri = parser.uri();

    // Claim the whole /api prefix.
    const auto suffix_opt = remove_prefix(uri, "/api/");
    if (!suffix_opt.has_value()) {
        return nullopt;
    }

    const auto suffix = *suffix_opt;

    if (!parser.authenticated()) {
        return StatusPage(Status::Unauthorized, parser.can_keep_alive());
    }

    const auto get_only = [parser](ConnectionState state) -> ConnectionState {
        if (parser.method == Method::Get) {
            return state;
        } else {
            // Drop the connection in fear there might be a body we don't know about.
            return StatusPage(Status::MethodNotAllowed, false);
        }
    };

    // Some stubs for now, to make more clients (including the web page) happier.
    if (suffix == "download") {
        return get_only(StatusPage(Status::NoContent, parser.can_keep_alive()));
    } else if (suffix == "settings") {
        return get_only(SendStaticMemory("{\"printer\": {}}", ContentType::ApplicationJson, parser.can_keep_alive()));
    } else if (remove_prefix(suffix, "files").has_value()) {
        if (parser.method == Method::Post) {
            auto upload = GcodeUpload::start(parser);
            /*
             * So, we have a "smaller" variant (eg. variant<A, B, C>) and
             * want a "bigger" variant<A, B, C, D, E>. C++ templates can't
             * do the "upgrade" automatically. But it can upgrade A into
             * the bigger one, it can upgrade B into the bigger one and can
             * upgrade C...
             *
             * Therefore we use the visit to take the first one apart and
             * then convert each type separately into the bigger one.
             */
            return std::visit([](auto upload) -> ConnectionState { return std::move(upload); }, std::move(upload));
        } else {
            /*
             * We do *not* use the uri with prefix removed. We need the safe
             * transformation into a file name (removal of query params,
             * forbidding of '..', etc).
             */
            /*
             * TODO:
             * We currently don't read the URI parameters. That is, we don't
             * understand "?recursive=true" and therefore don't do recursive.
             *
             * The FileInfo handler doesn't know recursive yet either.
             */
            static const auto prefix = "/api/files";
            static const size_t prefix_len = strlen(prefix);
            char fname[FILE_PATH_BUFFER_LEN + prefix_len];
            if (parser.uri_filename(fname, sizeof fname)) {
                size_t len = strlen(fname);
                if (prefix_len > len) {
                    return StatusPage(Status::NotFound, parser.can_keep_alive());
                }

                const char *fname_real = fname + prefix_len;

                /*
                 * The octoprint API gives special meaning to /local and
                 * /sdcard. For us, everything lives in the USB (/usb). We
                 * remap these. Nevertheless, we never _generate_ these /local
                 * or such URIs, so we don't remap anything else but the "root".
                 */
                static const char *const roots[] = {
                    "",
                    "/",
                    "/local",
                    "/local/",
                    "/sdcard",
                    "/sdcard/",
                };

                for (size_t i = 0; i < sizeof roots / sizeof roots[0]; i++) {
                    if (strcmp(fname_real, roots[i]) == 0) {
                        fname_real = "/usb/";
                        break;
                    }
                }

                /*
                 * Now, we make 100% sure the user won't get a file that's not
                 * on the USB drive (eg. our xflash).
                 */
                if (strncmp(fname_real, "/usb/", 5) != 0) {
                    StatusPage(Status::Forbidden, parser.can_keep_alive());
                }

                switch (parser.method) {
                case Method::Get:
                    return FileInfo(fname_real, parser.can_keep_alive(), false);
                case Method::Delete: {
                    int result = remove(fname_real);
                    if (result == -1) {
                        switch (errno) {
                        case EBUSY:
                            return StatusPage(Status::Conflict, parser.can_keep_alive(), "File is busy");
                        default:
                            return StatusPage(Status::NotFound, parser.can_keep_alive());
                        }
                    } else {
                        return StatusPage(Status::NoContent, parser.can_keep_alive());
                    }
                }
                default:
                    // Drop the connection in fear there might be a body we don't know about.
                    return StatusPage(Status::MethodNotAllowed, false);
                }
            } else {
                return StatusPage(Status::NotFound, parser.can_keep_alive());
            }
        }
        // The real API endpoints
    } else if (suffix == "version") {
        return get_only(GenOnce(handler_get_version, ContentType::ApplicationJson, parser.can_keep_alive()));
    } else if (suffix == "job") {
        switch (parser.method) {
        case Method::Get:
            return GenOnce(handler_get_job, ContentType::ApplicationJson, parser.can_keep_alive());
        case Method::Post: {
            if (parser.content_length.has_value()) {
                return JobCommand(*parser.content_length, parser.can_keep_alive());
            } else {
                // Drop the connection (and the body there).
                return StatusPage(Status::LengthRequired, false);
            }
        }
        default:
            return StatusPage(Status::MethodNotAllowed, false);
        }
    } else if (suffix == "printer") {
        return get_only(GenOnce(handler_get_printer, ContentType::ApplicationJson, parser.can_keep_alive()));
    } else {
        return StatusPage(Status::NotFound, parser.can_keep_alive());
    }
}

const PrusaLinkApi prusa_link_api;

}
