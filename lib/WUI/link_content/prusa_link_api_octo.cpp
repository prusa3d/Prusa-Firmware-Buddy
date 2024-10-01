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

Selector::Accepted PrusaLinkApiOcto::accept(const RequestParser &parser, Step &out) const {
    const string_view uri = parser.uri();

    // Claim the whole /api prefix.
    const auto suffix_opt = remove_prefix(uri, "/api/");
    if (!suffix_opt.has_value()) {
        return Accepted::NextSelector;
    }

    const auto suffix = *suffix_opt;

    if (!parser.check_auth(out)) {
        return Accepted::Accepted;
    }

    // Some stubs for now, to make more clients (including the web page) happier.
    if (suffix == "settings") {
        get_only(SendStaticMemory("{\"printer\": {}}", http::ContentType::ApplicationJson, parser.can_keep_alive()), parser, out);
        return Accepted::Accepted;
    } else if (remove_prefix(suffix, "files").has_value()) {
        // Note: The check for boundary is a bit of a hack. We probably should
        // be more thorough in the parser and extract the actual content type.
        // But that bit of the parser generator is a bit unreadable right now,
        // so we are lazy and do just this. It'll work fine in the correct
        // scenarios and will simply produce slightly weird error messages in
        // case it isn't.
        if (parser.method == Method::Post && !parser.boundary().empty()) {
            out.next = StatusPage(Status::Gone, parser);
            return Accepted::Accepted;
        } else {
            static const auto prefix = "/api/files";
            static const size_t prefix_len = strlen(prefix);
            char filename[FILE_PATH_BUFFER_LEN + prefix_len];
            if (!parse_file_url(parser, prefix_len, filename, sizeof(filename), RemapPolicy::Octoprint, out)) {
                return Accepted::Accepted;
            }

            switch (parser.method) {
            case Method::Get: {
                uint32_t etag = ChangedPath::instance.change_chain_hash(filename);
                out.next = FileInfo(filename, parser.can_keep_alive(), parser.accepts_json, false, FileInfo::ReqMethod::Get, FileInfo::APIVersion::Octoprint, etag);
                return Accepted::Accepted;
            }
            case Method::Delete: {
                out.next = delete_file(filename, parser);
                return Accepted::Accepted;
            }
            case Method::Post:
                if (parser.content_length.has_value()) {
                    out.next = FileCommand(filename, *parser.content_length, parser.can_keep_alive(), parser.accepts_json);
                } else {
                    // Drop the connection (and the body there).
                    out.next = StatusPage(Status::LengthRequired, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
                }
                return Accepted::Accepted;
            default:
                // Drop the connection in fear there might be a body we don't know about.
                out.next = StatusPage(Status::MethodNotAllowed, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
                return Accepted::Accepted;
            }
        }
        // The real API endpoints
    } else if (suffix == "version") {
        get_only(SendJson(EmptyRenderer(get_version), parser.can_keep_alive()), parser, out);
        return Accepted::Accepted;
    } else if (suffix == "job") {
        switch (parser.method) {
        case Method::Get:
            out.next = SendJson(EmptyRenderer(get_job_octoprint), parser.can_keep_alive());
            return Accepted::Accepted;
        case Method::Post: {
            if (parser.content_length.has_value()) {
                out.next = JobCommand(*parser.content_length, parser.can_keep_alive(), parser.accepts_json);
                return Accepted::Accepted;
            } else {
                // Drop the connection (and the body there).
                out.next = StatusPage(Status::LengthRequired, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
                return Accepted::Accepted;
            }
        }
        default:
            out.next = StatusPage(Status::MethodNotAllowed, StatusPage::CloseHandling::ErrorClose, parser.accepts_json);
            return Accepted::Accepted;
        }
    } else if (suffix == "printer") {
        get_only(SendJson(EmptyRenderer(get_printer), parser.can_keep_alive()), parser, out);
        return Accepted::Accepted;
    } else if (suffix == "download" || suffix == "transfer") {
        if (auto status = transfers::Monitor::instance.status(); status.has_value()) {
            get_only(SendJson(TransferRenderer(status->id, http::APIVersion::Octoprint), parser.can_keep_alive()), parser, out);
            return Accepted::Accepted;
        } else {
            out.next = StatusPage(Status::NoContent, parser);
            return Accepted::Accepted;
        }
    } else {
        out.next = StatusPage(Status::NotFound, parser);
        return Accepted::Accepted;
    }
}

const PrusaLinkApiOcto prusa_link_api_octo;

} // namespace nhttp::link_content
