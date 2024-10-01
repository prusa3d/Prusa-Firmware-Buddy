#include "send_file.h"
#include "handler.h"
#include "headers.h"

#include <http/chunked.h>

#include <sys/stat.h>

using std::nullopt;
using namespace http;

namespace nhttp::handler {

SendFile::SendFile(FILE *file, const char *path, ContentType content_type, bool can_keep_alive, [[maybe_unused]] bool json_errors, uint32_t if_none_match, const char *const *extra_hdrs)
    : file(file)
    , content_type(content_type)
    , can_keep_alive(can_keep_alive)
    , extra_hdrs(extra_hdrs) {
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

void SendFile::disable_caching() {
    etag_matches = false;
    etag = nullopt;
}

void SendFile::step(std::string_view, bool, uint8_t *buffer, size_t buffer_size, Step &out) {
    if (etag_matches) {
        // Note: json_errors are not enabled, because NotModified has no content anyway.
        out = Step { 0, 0, StatusPage(Status::NotModified, (connection_handling != ConnectionHandling::Close) ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, false, etag) };
        return;
    }

    if (!buffer) {
        return;
    }

    size_t written = 0;

    if (!headers_sent) {
        written = write_headers(buffer, buffer_size, Status::Ok, content_type, connection_handling, content_length, etag, extra_hdrs);
        headers_sent = true;

        if (written + MIN_CHUNK_SIZE >= buffer_size) {
            out = Step { 0, written, Continue() };
            return;
        }
        // Continue sending, there's still a bit of space left.
    }

    written += render_chunk(connection_handling, buffer + written, buffer_size - written, [&](uint8_t *buffer_, size_t buffer_size_) -> std::optional<size_t> {
        size_t read = fread(buffer_, 1, buffer_size_, file.get());
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
                out.next = Terminating {};
                return std::nullopt;
            } else {
                // This naturally returns and creates the terminating 0-sized chunk.
                out.next = Terminating::for_handling(connection_handling);
            }
            file.reset();
        }
        return read;
    });
    out.written = written;
}

} // namespace nhttp::handler
