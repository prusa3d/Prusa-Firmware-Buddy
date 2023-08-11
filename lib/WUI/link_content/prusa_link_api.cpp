#include "prusa_link_api.h"
#include "basic_gets.h"
#include "../nhttp/file_info.h"
#include "../nhttp/file_command.h"
#include "../nhttp/headers.h"
#include "../nhttp/gcode_upload.h"
#include "../nhttp/job_command.h"
#include "../nhttp/send_json.h"
#include "../wui_api.h"

#include <common/path_utils.h>
#include <transfers/monitor.hpp>
#include <transfers/changed_path.hpp>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <unistd.h>

namespace nhttp::link_content {

using http::Method;
using http::Status;
using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;
using nhttp::printer::FileCommand;
using nhttp::printer::FileInfo;
using nhttp::printer::GcodeUpload;
using nhttp::printer::JobCommand;
using transfers::ChangedPath;

using Type = ChangedPath::Type;
using Incident = ChangedPath::Incident;

enum class RemapPolicy {
    Octoprint,
    NoRemap
};

namespace {
    optional<string_view> remove_prefix(string_view input, string_view prefix) {
        if (input.size() < prefix.size() || input.substr(0, prefix.size()) != prefix) {
            return nullopt;
        }

        return input.substr(prefix.size());
    }

    optional<ConnectionState> parse_file_url(const RequestParser &parser, const size_t prefix_len, char *filename, const size_t filename_len, RemapPolicy remapPolicy) {
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
        const char *fname_real = filename + prefix_len;
        if (parser.uri_filename(filename, filename_len)) {
            size_t len = strlen(filename);
            if (prefix_len > len) {
                return StatusPage(Status::NotFound, parser);
            }

            if (remapPolicy == RemapPolicy::Octoprint) {
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
            }

            /*
             * Now, we make 100% sure the user won't get a file that's not
             * on the USB drive (eg. our xflash).
             */
            if (strncmp(fname_real, "/usb/", 5) != 0 && strcmp(fname_real, "/usb") != 0) {
                return StatusPage(Status::Forbidden, parser);
            }

            // We need to use memmove, because fname_real points into filename
            // and memmove explicitly allowes for overlapping buffer to be used
            size_t fname_real_len = strlen(fname_real);
            memmove(filename, fname_real, fname_real_len);
            filename[fname_real_len] = '\0';
            // Slicer sometimes produces duplicate slashes in the URL and
            // this may confuse eg. marlin.
            dedup_slashes(filename);
            return nullopt;
        } else {
            return StatusPage(Status::NotFound, parser);
        }
    }

    StatusPage delete_file(const char *filename, const RequestParser &parser) {
        int result = remove(filename);
        if (result == -1) {
            switch (errno) {
            case EBUSY:
                return StatusPage(Status::Conflict, parser, "File is busy");
            default:
                return StatusPage(Status::NotFound, parser);
            }
        } else {
            ChangedPath::instance.changed_path(filename, Type::File, Incident::Deleted);
            return StatusPage(Status::NoContent, parser);
        }
    }

    StatusPage print_file(char *filename, const RequestParser &parser) {
        // if not exists
        if (access(filename, R_OK) != 0) {
            return StatusPage(Status::NotFound, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
        }

        if (wui_start_print(filename, true) == StartPrintResult::PrintStarted) {
            // We ErrorClose the connection here, because theoretically
            // according to the API there could be a body, that we should
            // ignore. To ignore it explicitly, but with empty POST still working
            // ,would requier a lot more work. Closing it is not a big deal, because
            // starting a print does not happen very often.
            return StatusPage(Status::NoContent, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
        } else {
            return StatusPage(Status::Conflict, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
        }
    }
} // namespace

optional<ConnectionState> PrusaLinkApi::accept(const RequestParser &parser) const {
    // This is a little bit of a hack (similar one is in Connect). We want to
    // watch as often as possible if the USB is plugged in or not, to
    // invalidate dir listing caches in the browser. As we don't really have a
    // better place, we place it here.
    ChangedPath::instance.media_inserted(wui_media_inserted());

    const string_view uri = parser.uri();

    // Claim the whole /api prefix.
    const auto suffix_opt = remove_prefix(uri, "/api/");
    if (!suffix_opt.has_value()) {
        return nullopt;
    }

    const auto suffix = *suffix_opt;

    if (auto unauthorized_status = parser.authenticated_status(); unauthorized_status.has_value()) {
        return std::visit([](auto unauth_status) -> ConnectionState { return unauth_status; }, *unauthorized_status);
    }

    const auto get_only = [parser](ConnectionState state) -> ConnectionState {
        if (parser.method == Method::Get) {
            return state;
        } else {
            // Drop the connection in fear there might be a body we don't know about.
            return StatusPage(Status::MethodNotAllowed, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
        }
    };

    // Some stubs for now, to make more clients (including the web page) happier.
    if (suffix == "settings") {
        return get_only(SendStaticMemory("{\"printer\": {}}", http::ContentType::ApplicationJson, parser.can_keep_alive()));
    } else if (auto v1_suffix_opt = remove_prefix(suffix, "v1/"); v1_suffix_opt.has_value()) {
        const auto v1_suffix = *v1_suffix_opt;
        if (v1_suffix == "storage") {
            return get_only(SendJson(EmptyRenderer(get_storage), parser.can_keep_alive()));
        } else if (remove_prefix(v1_suffix, "files").has_value()) {
            static const auto prefix = "/api/v1/files";
            static const size_t prefix_len = strlen(prefix);
            char filename[FILE_PATH_BUFFER_LEN + prefix_len];
            auto error = parse_file_url(parser, prefix_len, filename, sizeof(filename), RemapPolicy::NoRemap);
            if (error.has_value()) {
                return error;
            }
            uint32_t etag = ChangedPath::instance.change_chain_hash(filename);
            if (etag == parser.if_none_match && etag != 0 /* 0 is special */ && (parser.method == Method::Get || parser.method == Method::Head)) {
                return StatusPage(Status::NotModified, parser.status_page_handling(), parser.accepts_json, etag);
            }
            switch (parser.method) {
            case Method::Put: {
                GcodeUpload::PutParams putParams;
                putParams.overwrite = parser.overwrite_file;
                putParams.print_after_upload = parser.print_after_upload;
                strlcpy(putParams.filepath.data(), filename, sizeof(putParams.filepath));
                auto upload = GcodeUpload::start(parser, wui_uploaded_gcode, parser.accepts_json, std::move(putParams));
                return std::visit([](auto upload) -> ConnectionState { return upload; }, std::move(upload));
            }
            case Method::Get: {
                return FileInfo(filename, parser.can_keep_alive(), parser.accepts_json, false, FileInfo::ReqMethod::Get, FileInfo::APIVersion::v1, etag);
            }
            case Method::Head: {
                return FileInfo(filename, parser.can_keep_alive(), parser.accepts_json, false, FileInfo::ReqMethod::Head, FileInfo::APIVersion::v1, etag);
            }
            case Method::Delete: {
                return delete_file(filename, parser);
            }
            case Method::Post: {
                return print_file(filename, parser);
            }
            default:
                return StatusPage(Status::MethodNotAllowed, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
            }
        } else {
            return StatusPage(Status::NotFound, parser);
        }
    } else if (remove_prefix(suffix, "files").has_value()) {
        // Note: The check for boundary is a bit of a hack. We probably should
        // be more thorough in the parser and extract the actual content type.
        // But that bit of the parser generator is a bit unreadable right now,
        // so we are lazy and do just this. It'll work fine in the correct
        // scenarios and will simply produce slightly weird error messages in
        // case it isn't.
        if (parser.method == Method::Post && !parser.boundary().empty()) {
            const auto boundary = parser.boundary();
            char boundary_cstr[boundary.size() + 1];
            memcpy(boundary_cstr, boundary.begin(), boundary.size());
            boundary_cstr[boundary.size()] = '\0';
            printer::UploadState uploadState(boundary_cstr);
            auto upload = GcodeUpload::start(parser, wui_uploaded_gcode, parser.accepts_json, std::move(uploadState));
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
            return std::visit([](auto upload) -> ConnectionState { return upload; }, std::move(upload));
        } else {
            static const auto prefix = "/api/files";
            static const size_t prefix_len = strlen(prefix);
            char filename[FILE_PATH_BUFFER_LEN + prefix_len];
            auto error = parse_file_url(parser, prefix_len, filename, sizeof(filename), RemapPolicy::Octoprint);
            if (error.has_value()) {
                return error;
            }

            switch (parser.method) {
            case Method::Get: {
                uint32_t etag = ChangedPath::instance.change_chain_hash(filename);
                return FileInfo(filename, parser.can_keep_alive(), parser.accepts_json, false, FileInfo::ReqMethod::Get, FileInfo::APIVersion::Octoprint, etag);
            }
            case Method::Delete: {
                return delete_file(filename, parser);
            }
            case Method::Post:
                if (parser.content_length.has_value()) {
                    return FileCommand(filename, *parser.content_length, parser.can_keep_alive(), parser.accepts_json);
                } else {
                    // Drop the connection (and the body there).
                    return StatusPage(Status::LengthRequired, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
                }
            default:
                // Drop the connection in fear there might be a body we don't know about.
                return StatusPage(Status::MethodNotAllowed, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
            }
        }
        // The real API endpoints
    } else if (suffix == "version") {
        return get_only(SendJson(EmptyRenderer(get_version), parser.can_keep_alive()));
    } else if (suffix == "job") {
        switch (parser.method) {
        case Method::Get:
            return SendJson(EmptyRenderer(get_job), parser.can_keep_alive());
        case Method::Post: {
            if (parser.content_length.has_value()) {
                return JobCommand(*parser.content_length, parser.can_keep_alive(), parser.accepts_json);
            } else {
                // Drop the connection (and the body there).
                return StatusPage(Status::LengthRequired, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
            }
        }
        default:
            return StatusPage(Status::MethodNotAllowed, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
        }
    } else if (suffix == "printer") {
        return get_only(SendJson(EmptyRenderer(get_printer), parser.can_keep_alive()));
    } else if (suffix == "download" || suffix == "transfer") {
        if (auto status = transfers::Monitor::instance.status(); status.has_value()) {
            return get_only(SendJson(TransferRenderer(status->id), parser.can_keep_alive()));
        } else {
            return StatusPage(Status::NoContent, parser);
        }
    } else {
        return StatusPage(Status::NotFound, parser);
    }
}

const PrusaLinkApi prusa_link_api;

} // namespace nhttp::link_content
