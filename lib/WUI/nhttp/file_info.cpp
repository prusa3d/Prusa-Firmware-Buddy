#include "chunked.h"
#include "file_info.h"
#include "handler.h"
#include "headers.h"
#include "status_page.h"
#include "../json_encode.h"
#include "../../src/common/lfn.h"
#include "../../src/common/gcode_filename.h"

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
    /*
     * Eat the last slash on directories.
     *
     * This is mostly aestetic only... otherwise we get double slashs in the
     * results/URLs. That works, but, meh...
     */
    const size_t len = strlen(this->filename);
    if (len > 0 && this->filename[len - 1] == '/') {
        this->filename[len - 1] = '\0';
    }
}

Step FileInfo::step(std::string_view, bool, uint8_t *output, size_t output_size) {
    /*
     * We assume the answer fits. If it doesn't, it doesn't overwrite anything, it just truncates the answer.
     * Directories send each file as a single packet, but that one also assumes it fits.
     */
    ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close;
    struct stat finfo;
    const size_t last_chunk_len = strlen(LAST_CHUNK);

    if (dir) {
        /*
         * We already have a directory listing running. Try to pull another file out of there.
         */
        struct dirent *ent;
        while ((ent = readdir(dir.get()))) {
            // Check this is GCODE, not some other thing. Skip the others.
            if (!filename_is_gcode(ent->d_name)) {
                continue;
            }

            size_t written = render_chunk(handling, output, output_size, [&](uint8_t *output, size_t output_size) {
                size_t written = 0;
                if (need_comma) {
                    assert(output_size > 0);
                    output[0] = ',';
                    written = 1;
                } else {
                    need_comma = true;
                }
                JSONIFY_STR(filename);
                const char *name = ent->d_name;
                JSONIFY_STR(name);
#ifdef UNITTESTS
                const char *long_name = ent->d_name;
#else
                const char *long_name = ent->lfn;
#endif
                JSONIFY_STR(long_name);
                written += snprintf(reinterpret_cast<char *>(output + written), output_size - written,
                    "{"
                    "\"name\":\"%s\","
                    "\"display\":\"%s\","
                    "\"path\": \"%s/%s\","
                    "\"origin\":\"local\","
                    "\"refs\":{"
                    "\"resource\":\"/api/files%s/%s\","
                    "\"thumbnailSmall\":\"/thumb/s%s/%s\","
                    "\"thumbnailBig\":\"/thumb/l%s/%s\","
                    "\"download\":\"%s/%s\""
                    "}"
                    "}",
                    name_escaped,
                    long_name_escaped,
                    filename_escaped, name_escaped,
                    filename_escaped, name_escaped,
                    filename_escaped, name_escaped,
                    filename_escaped, name_escaped,
                    filename_escaped, name_escaped);
                return written;
            });
            return Step { 0, written, Continue() };
        }
        // Run out of files, end the JSON and finish up.
        size_t written = render_chunk(handling, output, output_size - last_chunk_len, [&](uint8_t *output, size_t output_size) {
            return snprintf(reinterpret_cast<char *>(output), output_size, "]}");
        });
        if (can_keep_alive) {
            memcpy(output + written, LAST_CHUNK, last_chunk_len);
            written += last_chunk_len;
        }
        dir.reset();
        return Step { 0, written, Terminating::for_handling(handling) };
    }

    /*
     * Not having a directory listing running _yet_. So we try that. If it's
     * not a directory, this'll fail and we will try a file.
     */
    if (DIR *dir_attempt = opendir(filename); dir_attempt) {
        dir.reset(dir_attempt);

        size_t written = write_headers(output, output_size, after_upload ? Status::Created : Status::Ok, ContentType::ApplicationJson, handling);

        written += render_chunk(handling, output + written, output_size - written, [&](uint8_t *output, size_t output_size) {
            return snprintf(reinterpret_cast<char *>(output), output_size, "{\"files\": [");
        });
        return Step { 0, written, Continue() };
    } else {
        /*
         * Not a directory. Try a file.
         *
         * Files are one-shot, don't keep state for future.
         */
        int result = stat(filename, &finfo);
        if (result == 0) {
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
        } else if (strcmp(filename, "/usb") == 0) {
            // We are trying to list files in the root and it's not there -> USB is missing.
            // Special case it, we return empty list of files.
            size_t written = write_headers(output, output_size, after_upload ? Status::Created : Status::Ok, ContentType::ApplicationJson, handling);

            written += render_chunk(
                handling, output + written, output_size - written - last_chunk_len, [&](uint8_t *output, size_t output_size) {
                    return snprintf(reinterpret_cast<char *>(output), output_size, "{\"files\": []}");
                });

            assert(written + last_chunk_len <= output_size);
            if (can_keep_alive) {
                memcpy(output + written, LAST_CHUNK, last_chunk_len);
                written += last_chunk_len;
            }

            return Step { 0, written, Terminating::for_handling(handling) };
        } else {
            return Step { 0, 0, StatusPage(Status::NotFound, can_keep_alive) };
        }
    }
}

}
