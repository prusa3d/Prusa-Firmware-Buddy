#include "send_file.h"
#include "chunked.h"
#include "handler.h"
#include "headers.h"

#include <sys/stat.h>

namespace nhttp::handler {

SendFile::SendFile(FILE *file, const char *path, ContentType content_type, bool can_keep_alive, uint32_t if_none_match)
    : file(file)
    , content_type(content_type)
    , can_keep_alive(can_keep_alive) {
    struct stat finfo;
    if (stat(path, &finfo) == 0) {
        if (can_keep_alive) {
            connection_handling = ConnectionHandling::ContentLengthKeep;
        } else {
            connection_handling = ConnectionHandling::Close;
        }
        content_length = finfo.st_size;
        etag = compute_etag(finfo);
        if (etag == if_none_match && if_none_match != 0) {
            etag_matches = true;
        }
    } else if (can_keep_alive) {
        connection_handling = ConnectionHandling::ChunkedKeep;
    } else {
        connection_handling = ConnectionHandling::Close;
    }
}

Step SendFile::step(std::string_view, bool, uint8_t *buffer, size_t buffer_size) {
    if (etag_matches) {
        return Step { 0, 0, StatusPage(Status::NotModified, connection_handling != ConnectionHandling::Close) };
    }

    if (!buffer) {
        return Step { 0, 0, Continue() };
    }

    size_t written = 0;

    if (!headers_sent) {
        written = write_headers(buffer, buffer_size, Status::Ok, content_type, connection_handling, content_length, etag);
        headers_sent = true;

        if (written + MIN_CHUNK_SIZE >= buffer_size) {
            return Step { 0, written, Continue() };
        }
        // Continue sending, there's still a bit of space left.
    }

    NextInstruction instruction = Continue();
    written += render_chunk(connection_handling, buffer + written, buffer_size - written, [&](uint8_t *buffer, size_t buffer_size) -> std::optional<size_t> {
        size_t read = fread(buffer, 1, buffer_size, file.get());
        if (read == 0) {
            if (ferror(file.get())) {
                /*
                 * There was an error reading the file. This means it is potentially incomplete.
                 *
                 * We have already sent our 200-ok status. As a last effort
                 * to alert the caller about the problem, we terminate the
                 * connection abruptly without sending the termination
                 * chunk. That way the caller can figure out something is amiss.
                 *
                 * If we are in connection-close mode, then, well, there's
                 * no good way to signal that :-(.
                 */
                instruction = Terminating {};
                return std::nullopt;
            } else {
                // This naturally returns and creates the terminating 0-sized chunk.
                instruction = Terminating::for_handling(connection_handling);
            }
            file.reset();
        }
        return read;
    });
    return Step { 0, written, std::move(instruction) };
}

}
