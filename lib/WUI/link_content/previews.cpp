#include "previews.h"
#include "../nhttp/gcode_preview.h"
#include "../../../src/guiconfig/GuiDefaults.hpp"

#include <cstdio>

namespace nhttp::link_content {

using http::Method;
using http::Status;
using nhttp::printer::GCodePreview;
using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;

optional<ConnectionState> Previews::accept(const RequestParser &parser) const {
    const string_view uri = parser.uri();
    const constexpr string_view prefix = "/thumb/";

    // We claim the /thumb/ namespace.
    // The + 2 is for size specification (s/ or l/)
    const size_t extra_size = prefix.size() + 2;
    if (uri.size() < extra_size || uri.substr(0, prefix.size()) != prefix) {
        return nullopt;
    }

    // Content of the USB drive is only for authenticated, don't ever try anything without it.
    if (auto unauthorized_status = parser.authenticated_status(); unauthorized_status.has_value()) {
        return std::visit([](auto unauth_status) -> ConnectionState { return unauth_status; }, *unauthorized_status);
    }

    uint16_t width;
    uint16_t height;
    bool allow_larger;

    if (uri[prefix.size()] == 's') {
        width = 16;
        height = 16;
        allow_larger = false;
    } else if (uri[prefix.size()] == 'l') {
        // Anything bigger than 16
        width = 17;
        height = 17;
        allow_larger = true;
    } else {
        return StatusPage(Status::NotFound, parser, "Thumbnail size specification not recognized");
    }

    char fname[FILE_PATH_BUFFER_LEN + extra_size];
    if (!parser.uri_filename(fname, sizeof(fname))) {
        return StatusPage(Status::NotFound, parser, "This doesn't look like file name");
    }

    // Strip the extra prefix (without the last /)
    memmove(fname, fname + extra_size - 1, FILE_PATH_BUFFER_LEN);

    if (parser.method == Method::Get) {
        AnyGcodeFormatReader f(fname);
        if (f.is_open()) {
            return GCodePreview(std::move(f), fname, parser.can_keep_alive(), parser.accepts_json, width, height, allow_larger, parser.if_none_match);
        } else {
            return StatusPage(Status::NotFound, parser);
        }
    } else {
        return StatusPage(Status::MethodNotAllowed, parser);
    }
}

const Previews previews;

} // namespace nhttp::link_content
