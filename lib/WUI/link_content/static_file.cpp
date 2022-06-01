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
    constexpr const char prefix[] = "/internal/res/web";
    const size_t prefix_len = strlen(prefix);
    char fname_buffer[FILE_PATH_BUFFER_LEN];
    static_assert(sizeof(fname_buffer) > sizeof(prefix) + 1);
    const char *fname = fname_buffer;
    strcpy(fname_buffer, prefix);

    if (!parser.uri_filename(fname_buffer + prefix_len, sizeof(fname) - prefix_len)) {
        return nullopt;
    }

    if (fname[prefix_len] == '\0' || fname[prefix_len + 1] == '\0') {
        fname = "/internal/res/web/index.html";
    }

    FILE *f = fopen(fname, "rb");
    if (f) {
        static const char *extra_hdrs[] = {
            "Content-Encoding: gzip\r\n",
            "Cache-Control: private, max-age=86400\r\n",
            nullptr
        };
        return SendFile(f, fname, guess_content_by_ext(fname), parser.can_keep_alive(), parser.accepts_json, parser.if_none_match, extra_hdrs);
    }

    return nullopt;
}

const StaticFile static_file;

}
