#include "send_file.h"
#include "chunked.h"
#include "handler.h"
#include "headers.h"

namespace nhttp::handler {

Step SendFile::step(std::string_view, bool, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return Step { 0, 0, Continue() };
    }

    size_t written = 0;

    ConnectionHandling connection_handling = can_keep_alive ? ConnectionHandling::ChunkedKeep : ConnectionHandling::Close;
    if (!headers_sent) {
        written = write_headers(buffer, buffer_size, Status::Ok, content_type, connection_handling);
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
