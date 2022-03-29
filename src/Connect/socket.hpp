#pragma once

#include "connection.hpp"
#include "connect_error.h"

namespace con {

class socket_con : public virtual Connection {

private:
    int fd;
    bool connected;

public:
    socket_con();

    ~socket_con();

    std::optional<Error> connection(char *host, uint16_t port) override;

    std::variant<size_t, Error> tx(uint8_t *buffer, size_t data_len) override;

    std::variant<size_t, Error> rx(uint8_t *buffer, size_t buffer_len) override;
};

}
