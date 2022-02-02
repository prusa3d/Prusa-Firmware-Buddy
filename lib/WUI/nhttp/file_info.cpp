#include "chunked.h"
#include "file_info.h"
#include "handler.h"
#include "headers.h"
#include "status_page.h"
#include "../json_encode.h"
#include "../../src/common/lfn.h"

#include <cstring>
#include <sys/stat.h>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace nhttp::printer {

using namespace handler;

FileInfo::FileInfo(const char *filename, bool can_keep_alive, bool after_upload)
    : can_keep_alive(can_keep_alive)
    , after_upload(after_upload) {
    strlcpy(this->filename, filename, sizeof this->filename);
}

Step FileInfo::step(std::string_view, bool, uint8_t *output, size_t output_size) {
    // TODO: Support directories.

    // We assume the answer fits. If it doesn't, it doesn't overwrite anything, it just truncates the answer.
    // We'll have to revisit this for directories, of course.
    ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close;
    struct stat finfo;
    if (stat(filename, &finfo) == 0) {
        if (finfo.st_mode & S_IFDIR) {
            return Step { 0, 0, StatusPage(Status::NotImplemented, can_keep_alive, "Directory listing not implemented") };
        } else {
            const size_t last_chunk_len = strlen(LAST_CHUNK);
            size_t written = write_headers(output, output_size - last_chunk_len, after_upload ? Status::Created : Status::Ok, ContentType::ApplicationJson, handling);

            JSONIFY_STR(filename);
            char long_name[FILE_NAME_BUFFER_LEN];
            get_LFN(long_name, sizeof long_name, filename);
            JSONIFY_STR(long_name);

            written += render_chunk(
                handling, output + written, output_size - written - last_chunk_len, [&](uint8_t *output, size_t output_size) {
                    return snprintf(reinterpret_cast<char *>(output), output_size,
                        "{"
                        "\"name\":\"%s\","
                        "\"origin\":\"local\","
                        "\"size\":%zu,"
                        "\"refs\":{"
                        "\"resource\":\"/api/files%s\","
                        "\"thumbnailSmall\":\"/thumb/s%s\","
                        "\"thumbnailBig\":\"/thumb/l%s\","
                        "\"download\":\"%s\""
                        "}"
                        "}",
                        long_name_escaped, (size_t)finfo.st_size, filename_escaped, filename_escaped, filename_escaped, filename_escaped);
                });

            assert(written + last_chunk_len <= output_size);
            if (can_keep_alive) {
                memcpy(output + written, LAST_CHUNK, last_chunk_len);
                written += last_chunk_len;
            }

            return Step { 0, written, Terminating::for_handling(handling) };
        }
    } else {
        return Step { 0, 0, StatusPage(Status::NotFound, can_keep_alive) };
    }
}

}
