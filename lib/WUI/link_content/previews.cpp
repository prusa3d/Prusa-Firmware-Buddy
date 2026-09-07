#include "previews.h"
#include "../nhttp/gcode_preview.h"
#include <guiconfig/GuiDefaults.hpp>

#include <cstdio>
#include <buddy/filename_defs.hpp>

namespace nhttp::link_content {

using http::Method;
using http::Status;
using nhttp::printer::GCodePreview;
using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;

Selector::Accepted Previews::accept(const RequestParser &parser, handler::Step &out) const {
    const string_view uri = parser.uri();
    const constexpr string_view prefix = "/thumb/";

    // We claim the /thumb/ namespace.
    // The + 2 is for size specification (s/ or l/)
    const size_t extra_size = prefix.size() + 2;
    if (uri.size() < extra_size || uri.substr(0, prefix.size()) != prefix) {
        return Accepted::NextSelector;
    }

    // Content of the USB drive is only for authenticated, don't ever try anything without it.
    if (!parser.check_auth(out)) {
        return Accepted::Accepted;
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
        out.next = StatusPage(Status::NotFound, parser, "Thumbnail size specification not recognized");
        return Accepted::Accepted;
    }

    char fname[filename_defs::path_buffer_size + extra_size];
    if (!parser.uri_filename(fname, sizeof(fname))) {
        out.next = StatusPage(Status::NotFound, parser, "This doesn't look like file name");
        return Accepted::Accepted;
    }

    // Strip the extra prefix (without the last /)
    memmove(fname, fname + extra_size - 1, filename_defs::path_buffer_size);

    if (parser.method == Method::Get) {
        // Avoid copying over these guys on the stack, they are hundreds of bytes

        auto &state = out.next.emplace<ConnectionState>();
        auto &preview = state.emplace<GCodePreview>(fname, parser.can_keep_alive(), parser.accepts_json, width, height, allow_larger, parser.if_none_match);

        if (!preview.gcode_reader().open(fname)) {
            out.next = StatusPage(Status::NotFound, parser);
        }
        return Accepted::Accepted;
    } else {
        out.next = StatusPage(Status::MethodNotAllowed, parser);
        return Accepted::Accepted;
    }
}

const Previews previews;

} // namespace nhttp::link_content
