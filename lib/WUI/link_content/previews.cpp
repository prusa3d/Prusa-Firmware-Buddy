#include "previews.h"
#include "../nhttp/gcode_preview.h"

#include <cstdio>

namespace nhttp::link_content {

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
    if (!parser.authenticated()) {
        return StatusPage(Status::Unauthorized, parser.can_keep_alive());
    }

    uint16_t width;
    uint16_t height;

    if (uri[prefix.size()] == 's') {
        width = 16;
        height = 16;
    } else if (uri[prefix.size()] == 'l') {
        // TODO: Can we say "something bigger than 16" instead of hardcoding?
        width = 220;
        height = 140;
    } else {
        return StatusPage(Status::NotFound, parser.can_keep_alive(), "Thumbnail size specification not recognized");
    }

    char fname[FILE_PATH_BUFFER_LEN + extra_size];
    if (!parser.uri_filename(fname, sizeof(fname))) {
        return StatusPage(Status::NotFound, parser.can_keep_alive(), "This doesn't look like file name");
    }

    // Strip the extra prefix (without the last /)
    memmove(fname, fname + extra_size - 1, FILE_NAME_BUFFER_LEN);

    FILE *f = fopen(fname, "rb");

    if (f) {
        return GCodePreview(f, fname, parser.can_keep_alive(), width, height, parser.if_none_match);
    } else {
        return StatusPage(Status::NotFound, parser.can_keep_alive());
    }
}

const Previews previews;

}
