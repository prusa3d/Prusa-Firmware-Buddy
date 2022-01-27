#include "static_file.h"
#include "../nhttp/send_file.h"
#include "../nhttp/headers.h"

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

#include <cstring>

namespace nhttp::link_content {

using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;

optional<ConnectionState> StaticFile::accept(const RequestParser &parser) const {
    constexpr const char prefix[] = "/usb/www";
    const size_t prefix_len = strlen(prefix);
    char fname[FILE_PATH_BUFFER_LEN];
    static_assert(sizeof(fname) > sizeof(prefix));
    strcpy(fname, prefix);

    if (!parser.uri_filename(fname + prefix_len, sizeof(fname) - prefix_len)) {
        return nullopt;
    }

    FILE *f = fopen(fname, "rb");
    if (f) {
        return SendFile(f, fname, guess_content_by_ext(fname), parser.can_keep_alive());
    }

    return nullopt;
}

const StaticFile static_file;

}
