#include "gen_once.h"
#include "handler.h"
#include "headers.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace nhttp::handler {
// TODO: Move these somewhere for sharing with others
namespace {

    const constexpr char *const LAST_CHUNK = "0\r\n\r\n";

    template <typename Renderer>
    size_t render_chunk(ConnectionHandling handling, uint8_t *buffer, size_t buffer_len, Renderer &&renderer) {
        if (handling == ConnectionHandling::ChunkedKeep) {
            /*
             * We cheat a bit here. We leave a space for the size. We assume the buffer
             * is not longer than 2^16 (if it is, we truncate it). Therefore, we can be
             * sure the size won't have more than 5 hex digits. We pad it with zeroes
             * (which should be OK) to have the same size.
             *
             * Therefore we can just put the big chunk at exact place and not move it
             * later on.
             */
            const size_t skip = 6; // 4 digits + \r\n
            const size_t tail = 2; // \r\n at the end
            assert(buffer_len >= skip + tail);

            size_t written = renderer(buffer + skip, std::min(buffer_len - skip - tail, static_cast<size_t>(0xffff)));
            // Render the header separately so we don't overwrite the buffer with \0
            char header[skip + 1];
            const size_t header_size = snprintf(header, sizeof(header), "%04zX\r\n", written);
            assert(header_size == skip);

            memcpy(buffer, header, header_size);
            memcpy(buffer + skip + written, "\r\n", tail);

            return skip + written + tail;
        } else {
            return renderer(buffer, buffer_len);
        }
    }

}

Step GenOnce::step(std::string_view, bool, uint8_t *buffer, size_t buffer_size) {
    const size_t last_chunk_len = strlen(LAST_CHUNK);
    size_t written = 0;
    switch (progress) {
    case Progress::SendHeaders:
        written = write_headers(buffer, buffer_size, Status::Ok, content_type, connection_handling);
        progress = Progress::SendPayload;
        return Step { 0, written, Continue() };
        // We do _not_ fall through here. We want to give the generator as
        // much space as possible, because we don't know how much it'll
        // need.
    case Progress::SendPayload:
        written = render_chunk(connection_handling, buffer, buffer_size, generator);
        progress = Progress::EndChunk;
        if (connection_handling != ConnectionHandling::ChunkedKeep) {
            progress = Progress::Done;
            return Step { 0, written, Terminating::for_handling(connection_handling) };
        }
        if (buffer_size - written < last_chunk_len) {
            return Step { 0, written, Continue() };
        }
        // Fall through: the last chunk does fit.
    case Progress::EndChunk:
        assert(buffer_size - written >= last_chunk_len);
        memcpy(buffer + written, LAST_CHUNK, last_chunk_len);
        progress = Progress::Done;
        return Step { 0, written + last_chunk_len, Terminating::for_handling(connection_handling) };
    case Progress::Done:
    default:
        assert(false);
        return Step { 0, 0, Continue() };
    }
}

}
