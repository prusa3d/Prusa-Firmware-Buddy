#pragma once

#include <http/connection.hpp>
#include <http/connect_error.h>

#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/certs.h>
#include <mbedtls/platform.h>

#include "net_sockets.hpp"

namespace connect_client {

class tls final : public http::Connection {

private:
    mbedtls_ssl_config ssl_config;
    mbedtls_net_context net_context;
    mbedtls_ssl_context ssl_context;
    bool custom_cert;

public:
    tls(uint8_t timeout_s, bool custom_cert);
    ~tls();
    tls(const tls &other) = delete;
    tls(tls &&other) = delete;
    tls &operator=(const tls &other) = delete;
    tls &operator=(tls &&other) = delete;

    virtual std::optional<http::Error> connection(const char *host, uint16_t port) override;
    virtual std::variant<size_t, http::Error> tx(const uint8_t *buffer, size_t data_len) override;
    virtual std::variant<size_t, http::Error> rx(uint8_t *buffer, size_t buffer_len, bool nonblock) override;
    virtual bool poll_readable(uint32_t timeout) override;
};

} // namespace connect_client
