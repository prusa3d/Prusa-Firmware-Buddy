#include "socket_connection_factory.hpp"

namespace http {
SocketConnectionFactory::SocketConnectionFactory(const char *host, uint16_t port, uint8_t timeout_s)
    : hostname(host)
    , port(port)
    , con(timeout_s) {
}

std::variant<Connection *, Error> SocketConnectionFactory::connection() {
    if (auto rc = con.connection(hostname, port); rc.has_value()) {
        return rc.value();
    }
    return &con;
}
const char *SocketConnectionFactory::host() {
    return hostname;
}
void SocketConnectionFactory::invalidate() {}
}; // namespace http
