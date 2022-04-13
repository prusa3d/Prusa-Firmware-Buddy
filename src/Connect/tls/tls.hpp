#pragma once

#include "connection.hpp"
#include "connect_error.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/certs.h"
#include "mbedtls/platform.h"

namespace con {

class tls final : public Connection {

private:
    mbedtls_ssl_config ssl_config;
    mbedtls_net_context net_context;
    mbedtls_ssl_context ssl_context;

public:
    tls();
    ~tls();
    tls(const tls &other) = delete;
    tls(tls &&other) = delete;
    tls &operator=(const tls &other) = delete;
    tls &operator=(tls &&other) = delete;

    virtual std::optional<Error> connection(char *host, uint16_t port) override;
    virtual std::variant<size_t, Error> tx(uint8_t *buffer, size_t data_len) override;
    virtual std::variant<size_t, Error> rx(uint8_t *buffer, size_t buffer_len) override;
};

}
