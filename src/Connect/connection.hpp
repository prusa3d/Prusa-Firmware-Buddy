#pragma once

#include <optional>
#include <variant>
#include <cstdint>
#include "connect_error.h"

namespace con {

constexpr uint8_t SOCKET_TIMEOUT_SEC = 30;

class Connection {
public:
    virtual std::optional<Error> connection(char *host, uint16_t port) = 0;
    virtual std::variant<size_t, Error> rx(uint8_t *buffer, size_t len) = 0;
    virtual std::variant<size_t, Error> tx(uint8_t *buffer, size_t len) = 0;
    virtual ~Connection() = default;
};

}
