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

class tls : public virtual Connection {

private:
    mbedtls_ssl_config ssl_config;
    mbedtls_net_context net_context;
    mbedtls_ssl_context ssl_context;

public:
    tls();

    ~tls();

    std::optional<Error> connect(char *host, uint16_t port);

    std::variant<size_t, Error> write(uint8_t *buffer, size_t data_len);

    std::variant<size_t, Error> read(uint8_t *buffer, size_t buffer_len);
};

}
