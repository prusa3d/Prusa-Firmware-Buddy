#pragma once

#include "connection.hpp"
#include "connect_error.h"

namespace http {

class socket_con final : public Connection {

private:
    int fd;
    bool connected;

public:
    socket_con(uint8_t timeout_s);
    ~socket_con();
    socket_con(const socket_con &other);
    socket_con(socket_con &&other);
    socket_con &operator=(const socket_con &other) = delete;
    socket_con &operator=(socket_con &&other) = delete;

    virtual std::optional<Error> connection(const char *host, uint16_t port) override;
    virtual std::variant<size_t, Error> tx(const uint8_t *buffer, size_t data_len) override;
    virtual std::variant<size_t, Error> rx(uint8_t *buffer, size_t buffer_len, bool nonblock) override;
    virtual bool poll_readable(uint32_t timeout) override;
};

} // namespace http
