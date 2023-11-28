#include "prusa_link_api_octo.h"
#include "basic_gets.h"
#include "../nhttp/file_info.h"
#include "../nhttp/file_command.h"
#include "../nhttp/headers.h"
#include "../nhttp/gcode_upload.h"
#include "../nhttp/job_command.h"
#include "../nhttp/send_json.h"
#include "../wui_api.h"
#include "prusa_api_helpers.hpp"

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

optional<ConnectionState> PrusaLinkApiOcto::accept(const RequestParser &parser) const {
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

    // Some stubs for now, to make more clients (including the web page) happier.
    if (suffix == "settings") {
        return get_only(SendStaticMemory("{\"printer\": {}}", http::ContentType::ApplicationJson, parser.can_keep_alive()), parser);
    } else if (remove_prefix(suffix, "files").has_value()) {
        // Note: The check for boundary is a bit of a hack. We probably should
        // be more thorough in the parser and extract the actual content type.
        // But that bit of the parser generator is a bit unreadable right now,
        // so we are lazy and do just this. It'll work fine in the correct
        // scenarios and will simply produce slightly weird error messages in
        // case it isn't.
        if (parser.method == Method::Post && !parser.boundary().empty()) {
            const auto boundary = parser.boundary();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla" // TODO: person who knows a reasonable buffer size should refactor this code to not use variable length array
            char boundary_cstr[boundary.size() + 1];
#pragma GCC diagnostic pop
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
        return get_only(SendJson(EmptyRenderer(get_version), parser.can_keep_alive()), parser);
    } else if (suffix == "job") {
        switch (parser.method) {
        case Method::Get:
            return SendJson(EmptyRenderer(get_job_octoprint), parser.can_keep_alive());
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
        return get_only(SendJson(EmptyRenderer(get_printer), parser.can_keep_alive()), parser);
    } else if (suffix == "download" || suffix == "transfer") {
        if (auto status = transfers::Monitor::instance.status(); status.has_value()) {
            return get_only(SendJson(TransferRenderer(status->id, http::APIVersion::Octoprint), parser.can_keep_alive()), parser);
        } else {
            return StatusPage(Status::NoContent, parser);
        }
    } else {
        return StatusPage(Status::NotFound, parser);
    }
}

const PrusaLinkApiOcto prusa_link_api_octo;

} // namespace nhttp::link_content
