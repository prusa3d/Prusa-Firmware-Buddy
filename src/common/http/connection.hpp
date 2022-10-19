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
    virtual std::variant<size_t, Error> rx(uint8_t *buffer, size_t len) = 0;
    virtual std::variant<size_t, Error> tx(const uint8_t *buffer, size_t len) = 0;
    virtual ~Connection() = default;

    uint8_t get_timeout_s() const;

private:
    uint8_t timeout_s;
};

}
