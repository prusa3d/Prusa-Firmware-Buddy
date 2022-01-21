#include "static_fs_file.h"
#include "../nhttp/static_mem.h"
#include "../nhttp/headers.h"

#include <lwip/apps/fs.h>

#include <cstring>

namespace nhttp::link_content {

using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;

optional<ConnectionState> StaticFsFile::accept(const RequestParser &parser) const {
    char filename[MAX_URL_LEN + 1];

    if (!parser.uri_filename(filename, sizeof(filename))) {
        return nullopt;
    }

    if (strcmp(filename, "/") == 0) {
        strncpy(filename, "/index.html", MAX_URL_LEN);
    }

    fs_file file;
    if (fs_open(&file, filename) == ERR_OK) {
#if LWIP_HTTPD_CUSTOM_FILES
        // These are not static chunk of memory, not supported.
        if (file.is_custom_file) {
            fs_close(&file);
            return nullopt;
        }
#endif
        /*
         * The data of non-custom fs files are embedded inside the program
         * as C arrays. Therefore, we can get the data and close the file
         * right away.
         */
        /*
         * We guess the content type by a file extension. Looking inside
         * and doing "file magic" would be better/more reliable, but don't
         * want the code complexity of that.
         */
        static const char *extra_hdrs[] = { "Content-Encoding: gzip\r\n", nullptr };
        SendStaticMemory send(string_view(file.data, file.len), guess_content_by_ext(filename), parser.can_keep_alive(), extra_hdrs);
        fs_close(&file);
        return send;
    }

    return nullopt;
}

const StaticFsFile static_fs_file;

}
