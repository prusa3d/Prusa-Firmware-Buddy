#pragma once

#include "connection.hpp"

#include <memory>

namespace http {

struct ReadBuffer {
    static constexpr uint8_t MAX_BUFF = 64;
    uint8_t size;
    uint8_t data[MAX_BUFF];
};

/// A connection wrapper to hold a bit of early data.
///
/// The early data is returned first, then the actual connection is used from that point on.
///
/// The inner connection is not owned (the caller is responsible for deleting).
///
/// The fact this is derived from Connection is to reuse code (there are
/// several non-virtual methods building on top of these), and we can reuse the
/// algorithms and simply hide the fact some of the data is already
/// pre-received.
class BufferedConnection final : public Connection {
private:
    Connection *inner;
    std::unique_ptr<ReadBuffer> read_buffer;

public:
    BufferedConnection(Connection *connection, const uint8_t *buff, size_t len);
    virtual std::optional<Error> connection(const char *, uint16_t) override;
    virtual std::variant<size_t, Error> rx(uint8_t *buffer, size_t len, bool nonblock) override;
    virtual std::variant<size_t, Error> tx(const uint8_t *buffer, size_t len) override;
    virtual bool poll_readable(uint32_t timeout) override;
};

} // namespace http
