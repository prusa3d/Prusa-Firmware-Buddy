#include "buffered_conn.hpp"

#include <cassert>
#include <cstring>

using std::min;
using std::optional;
using std::variant;

namespace http {

BufferedConnection::BufferedConnection(Connection *connection, const uint8_t *buff, size_t len)
    : Connection(connection->get_timeout_s())
    , inner(connection) {
    if (len > 0) {
        read_buffer.reset(new ReadBuffer);
        assert(len <= ReadBuffer::MAX_BUFF);
        read_buffer->size = len;
        memcpy(read_buffer->data, buff, len);
    }
}

optional<Error> BufferedConnection::connection(const char *, uint16_t) {
    assert(0); // This one is used to wrap another connection after connecting.
    return Error::InternalError;
}

variant<size_t, Error> BufferedConnection::rx(uint8_t *buffer, size_t len, bool nonblock) {
    if (read_buffer) {
        size_t to_read = min(len, static_cast<size_t>(read_buffer->size));
        memcpy(buffer, read_buffer->data, to_read);
        read_buffer->size -= to_read;
        if (read_buffer->size > 0) {
            memmove(read_buffer->data, read_buffer->data + to_read, read_buffer->size);
        } else {
            read_buffer.reset();
        }

        return to_read;
    }

    return inner->rx(buffer, len, nonblock);
}

variant<size_t, Error> BufferedConnection::tx(const uint8_t *buffer, size_t len) {
    return inner->tx(buffer, len);
}

bool BufferedConnection::poll_readable(uint32_t timeout) {
    return read_buffer || inner->poll_readable(timeout);
}

} // namespace http
