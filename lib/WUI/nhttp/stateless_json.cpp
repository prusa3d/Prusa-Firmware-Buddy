#include "stateless_json.h"
#include "chunked.h"
#include "handler.h"
#include "headers.h"

namespace nhttp::handler {

JsonResult StatelessJson::Renderer::renderState(size_t resume_point, JsonOutput &output, Empty &state) const {
    return generator(resume_point, output);
}

Step StatelessJson::step(std::string_view, bool, uint8_t *buffer, size_t buffer_size) {
    const size_t last_chunk_len = strlen(LAST_CHUNK);
    size_t written = 0;
    bool first_packet = false;
    switch (progress) {
    case Progress::SendHeaders:
        written = write_headers(buffer, buffer_size, Status::Ok, ContentType::ApplicationJson, connection_handling);
        progress = Progress::SendPayload;
        first_packet = true;

        if (buffer_size - written <= MIN_CHUNK_SIZE) {
            return { 0, written, Continue() };
        }
        // Fall through, see if something more fits.
    case Progress::SendPayload:
        JsonResult render_result;

        written += render_chunk(connection_handling, buffer + written, buffer_size - written, [&](uint8_t *buffer, size_t buffer_size) {
            // TODO: The actual renderer goes somewhere!
            const auto [result, written_json] = renderer.render(buffer, buffer_size);
            render_result = result;
            return written_json;
        });

        switch (render_result) {
        case JsonResult::Complete:
            progress = Progress::EndChunk;

            break;
        case JsonResult::Incomplete:
            // Send this packet out, but ask for another one.
            return { 0, written, Continue() };
        case JsonResult::BufferTooSmall:
            // It is small, but we've alreaty taken part of it by headers. Try
            // again with the next packet.
            if (first_packet) {
                return { 0, written, Continue() };
            }
            // Fall through to the error state.
        case JsonResult::Abort:
            // Something unexpected got screwed up. We don't have a way to
            // return a 500 error, we have sent the headers out already
            // (possibly), so the best we can do is to abort the
            // connection.
            return { 0, 0, Terminating { 0, Done::CloseFast } };
        }

        // Fall through: the last chunk may fit
    case Progress::EndChunk:
        if (connection_handling == ConnectionHandling::ChunkedKeep) {
            if (written + last_chunk_len > buffer_size) {
                // Need to leave the last chunk for next packet
                return { 0, written, Continue() };
            } else {
                memcpy(buffer + written, LAST_CHUNK, last_chunk_len);
                written += last_chunk_len;
            }
        }

        progress = Progress::Done;

        return Step { 0, written, Terminating::for_handling(connection_handling) };
    case Progress::Done:
    default:
        assert(false);
        return Step { 0, 0, Continue() };
    }
}

}
