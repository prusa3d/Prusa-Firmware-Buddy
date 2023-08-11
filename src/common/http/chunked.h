#include "types.h"

#include <cassert>
#include <cstdbool>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <optional>

namespace http {

const constexpr char *const LAST_CHUNK = "0\r\n\r\n";
const constexpr size_t MIN_CHUNK_SIZE = 8;

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
        assert(buffer_len > skip + tail);

        const size_t available = std::min(buffer_len - skip - tail, static_cast<size_t>(0xffff));
        std::optional<size_t> written = renderer(buffer + skip, available);
        if (!written.has_value()) {
            // No chunk to render, bail out.
            return 0;
        } else {
            assert(*written <= available);
            // Bug mitigation. If the renderer reports to have written more
            // than the buffer size - as easily done with snprintf, which
            // reports how much it _wanted_ to write even if the buffer is too
            // small - make sure not to go out of range. That's still a bug,
            // therefore the assert above that.
            written = std::min(*written, available);
        }
        // Render the header separately so we don't overwrite the buffer with \0
        char header[skip + 1];
        const size_t header_size = snprintf(header, sizeof(header), "%04zX\r\n", *written);
        assert(header_size == skip);

        memcpy(buffer, header, header_size);
        memcpy(buffer + skip + *written, "\r\n", tail);

        return skip + *written + tail;
    } else {
        std::optional<size_t> written = renderer(buffer, buffer_len);
        return written.value_or(0);
    }
}

} // namespace http
