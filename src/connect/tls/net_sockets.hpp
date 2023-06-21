#pragma once

#include <common/http/socket.hpp>
#include <mbedtls/net_sockets.h>

struct mbedtls_net_context {
    http::socket_con plain_conn;
    bool timeout_happened;
    mbedtls_net_context(uint8_t timeout_s);
    mbedtls_net_context(const mbedtls_net_context &other) = delete;
    mbedtls_net_context(mbedtls_net_context &&other) = delete;
    mbedtls_net_context &operator=(const mbedtls_net_context &other) = delete;
    mbedtls_net_context &operator=(mbedtls_net_context &&other) = delete;
};
