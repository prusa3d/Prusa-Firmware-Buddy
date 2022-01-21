#include "gen_once.h"
#include "chunked.h"
#include "handler.h"
#include "headers.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace nhttp::handler {

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
