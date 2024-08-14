#include "connection_cache.hpp"

#include <debug.h>

using http::Connection;
using http::Error;
using http::socket_con;
using std::get;
using std::holds_alternative;
using std::monostate;
using std::optional;

LOG_COMPONENT_REF(connect);

namespace {

inline constexpr uint8_t SOCKET_TIMEOUT_SEC = 5;

}

namespace connect_client {

std::variant<Connection *, Error> CachedFactory::connection() {
    // Note: The monostate state should not be here at this moment, it's only after invalidate and similar.
    if (Connection *c = get_if<tls>(&cache); c != nullptr) {
        return c;
    } else if (c = get_if<socket_con>(&cache); c != nullptr) {
        return c;
    } else {
        Error error = get<Error>(cache);
        // Error is just one-off. Next time we'll try connecting again.
        cache = monostate();
        return error;
    }
}

const char *CachedFactory::host() {
    return hostname;
}

void CachedFactory::invalidate() {
    log_debug(connect, "Invalidating cached connection");
    cache = monostate();
}

void CachedFactory::refresh(const Printer::Config &config) {
    hostname = config.host;
    if (holds_alternative<monostate>(cache)) {
        Connection *connection;

        if (config.tls) {
            log_debug(connect, "Creating TLS");
            cache.emplace<tls>(SOCKET_TIMEOUT_SEC, config.custom_cert);
            connection = &get<tls>(cache);
        } else {
            log_debug(connect, "Creating plain");
            cache.emplace<socket_con>(SOCKET_TIMEOUT_SEC);
            connection = &get<socket_con>(cache);
        }
        if (const optional<Error> result = connection->connection(config.host, config.port); result.has_value()) {
            log_info(connect, "Creating of connection failed: %s", http::to_str(result.value()));
            cache = *result;
        }
    } else {
        log_debug(connect, "Reusing existing connection");
    }
}

bool CachedFactory::is_valid() const {
    return holds_alternative<tls>(cache) || holds_alternative<socket_con>(cache);
}

} // namespace connect_client
