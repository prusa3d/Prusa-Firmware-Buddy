#pragma once
#include "httpc.hpp"
#include "socket.hpp"

namespace http {
class SocketConnectionFactory : public ConnectionFactory {
public:
    SocketConnectionFactory(const char *host, uint16_t port, uint8_t timeout_s);
    virtual ~SocketConnectionFactory() = default;

protected:
    std::variant<Connection *, Error> connection() override;
    const char *host() override;
    void invalidate() override;

private:
    const char *hostname;
    uint16_t port;
    socket_con con;
};
}; // namespace http
