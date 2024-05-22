#pragma once

#include <optional>
#include <variant>
#include <cstdint>
#include "connect_error.h"

namespace http {

class Connection {
public:
    Connection(uint8_t timeout_s);
    virtual std::optional<Error> connection(const char *host, uint16_t port) = 0;
    virtual std::variant<size_t, Error> rx(uint8_t *buffer, size_t len, bool nonblock) = 0;
    virtual std::variant<size_t, Error> tx(const uint8_t *buffer, size_t len) = 0;

    std::optional<Error> tx_all(const uint8_t *buffer, size_t len);
    std::optional<Error> rx_exact(uint8_t *buffer, size_t len);

    /// Wait for it to become readable, but for max timeout ms.
    ///
    /// Returns true if readable.
    /// (there's a strong suspicion there might be cases of false positive).
    virtual bool poll_readable(uint32_t timeout) = 0;
    virtual ~Connection() = default;

    uint8_t get_timeout_s() const;

private:
    uint8_t timeout_s;
};

} // namespace http
