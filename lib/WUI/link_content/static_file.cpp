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

namespace {
    constexpr const string_view INDEX = "index.html";
}

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

    bool cache_enabled = true;

    if (fname[prefix_len] == '\0' || fname[prefix_len + 1] == '\0') {
        fname = "/internal/res/web/index.html";
    }

    // Note: The index.html might change if we reflash something. Then the old
    // (cached) version could link to no longer existing css and js files.
    const string_view fname_sv(fname);
    if (fname_sv.size() >= INDEX.size() && fname_sv.rfind(INDEX) == fname_sv.size() - INDEX.size()) {
        cache_enabled = false;
    }

    FILE *f = fopen(fname, "rb");
    if (f) {
        static const char *extra_hdrs_cache[] = {
            "Content-Encoding: gzip\r\n",
            "Cache-Control: private, max-age=86400\r\n",
            nullptr
        };
        static const char *extra_hdrs_no_cache[] = {
            "Content-Encoding: gzip\r\n",
            nullptr
        };
        return SendFile(f, fname, guess_content_by_ext(fname), parser.can_keep_alive(), parser.accepts_json, parser.if_none_match, cache_enabled ? extra_hdrs_cache : extra_hdrs_no_cache);
    }

    return nullopt;
}

const StaticFile static_file;

}
