#pragma once

#include <optional>
#include <variant>
#include "connect_error.h"

namespace con {

class Connection {

public:
    virtual std::optional<Error> connect(char *host, uint16_t port) = 0;
    virtual std::variant<size_t, Error> read(uint8_t *buffer, size_t len) = 0;
    virtual std::variant<size_t, Error> write(uint8_t *buffer, size_t len) = 0;
    virtual ~Connection() = default;
};

}
