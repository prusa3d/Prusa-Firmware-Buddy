#include "tls.hpp"
#include "certificate.h"
#include <string.h>
#include <stdbool.h>

using http::Error;

namespace {

struct InitContexts {
    mbedtls_x509_crt x509_certificate;
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context drbg_context;
    InitContexts() {
        mbedtls_x509_crt_init(&x509_certificate);
        mbedtls_entropy_init(&entropy_context);
        mbedtls_ctr_drbg_init(&drbg_context);
    }
    InitContexts(const InitContexts &others) = delete;
    InitContexts(InitContexts &&others) = delete;
    InitContexts &operator=(const InitContexts &others) = delete;
    InitContexts &operator=(InitContexts &&others) = delete;
    ~InitContexts() {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
    }
};

} // namespace

namespace connect_client {

tls::tls(uint8_t timeout_s)
    : http::Connection(timeout_s)
    , net_context(timeout_s) {
    mbedtls_net_init(&net_context);
    mbedtls_ssl_init(&ssl_context);
    mbedtls_ssl_config_init(&ssl_config);
}

tls::~tls() {
    // We currently do _not_ do this on purpose even though it's a good practice, because:
    // * This'll create and try to send another packet over the TCP.
    // * Nevertheless, we do this in case we think the TCP connection is _dead_.
    //
    // So the likely effect is just allocating more memory for the packet
    // that'll retransmit multiple times and sit there for a long time, instead
    // just getting rid of it. Even in the case where we _would_ send it
    // successfully, it doesn't serve any particular purpose for us (we are
    // between full request-response exchanges at this point, so there's
    // nothing to effectively close/confirm).
    // mbedtls_ssl_close_notify(&ssl_context);
    mbedtls_net_free(&net_context);
    mbedtls_ssl_free(&ssl_context);
    mbedtls_ssl_config_free(&ssl_config);
}

std::optional<Error> tls::connection(const char *host, uint16_t port) {

    int status;
    InitContexts ctxs;

    if ((status = mbedtls_ctr_drbg_seed(&ctxs.drbg_context, mbedtls_entropy_func, &ctxs.entropy_context, NULL, 0)) != 0) {
        return Error::InternalError;
    }

    mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &ctxs.drbg_context);
    if ((status = mbedtls_x509_crt_parse_der_nocopy(&ctxs.x509_certificate, (const unsigned char *)ca_cert_der, ca_cert_der_len))
        != 0) {
        return Error::InternalError;
    }

    mbedtls_ssl_conf_ca_chain(&ssl_config, &ctxs.x509_certificate, NULL);

    if ((status = mbedtls_ssl_config_defaults(&ssl_config,
             MBEDTLS_SSL_IS_CLIENT,
             MBEDTLS_SSL_TRANSPORT_STREAM,
             MBEDTLS_SSL_PRESET_DEFAULT))
        != 0) {
        return Error::InternalError;
    }

    // Only use TLS 1.2
    mbedtls_ssl_conf_max_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    // Strictly ensure that certificates are signed by the CA
    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);

    // set cipher suite to use
    static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, 0 };
    mbedtls_ssl_conf_ciphersuites(&ssl_config, tls_cipher_suites);

    mbedtls_ssl_set_hostname(&ssl_context, host);

    if ((status = mbedtls_ssl_setup(&ssl_context, &ssl_config)) != 0) {
        return Error::InternalError;
    }

    mbedtls_ssl_set_bio(&ssl_context, &net_context, mbedtls_net_send, mbedtls_net_recv, NULL);

    constexpr size_t str_len = 6;
    char port_as_str[str_len] = {};
    snprintf(port_as_str, str_len, "%hu", port);

    if ((status = mbedtls_net_connect(&net_context, host, port_as_str, MBEDTLS_NET_PROTO_TCP)) != 0) {
        return Error::Connect;
    }

    while ((status = mbedtls_ssl_handshake(&ssl_context)) != 0) {
        if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return Error::Tls;
        }

        if (net_context.timeout_happened) {
            // Timeouts are mapped to ERR_SSL_WANT_(READ|WRITE). But possibly
            // there are other things that are mapped to that too? Not sure.
            // Therefore, we smuggle the timeouts in this side channel.
            //
            // This is set in mbedtls_net_recv/mbedtls_net_send in
            // net_sockets.cpp
            return Error::Timeout;
        }
    }

    if ((status = mbedtls_ssl_get_verify_result(&ssl_context)) != 0) {
        return Error::Tls;
    }

    return std::nullopt;
}

std::variant<size_t, Error> tls::tx(const uint8_t *send_buffer, size_t data_len) {
    size_t bytes_sent = 0;

    int status = mbedtls_ssl_write(&ssl_context, (const unsigned char *)send_buffer, data_len);

    if (status <= 0) {
        if (net_context.timeout_happened) {
            return Error::Timeout;
        } else {
            return Error::Network;
        }
    }

    bytes_sent = (size_t)status;
    return bytes_sent;
}

std::variant<size_t, Error> tls::rx(uint8_t *read_buffer, size_t buffer_len, [[maybe_unused]] bool nonblock) {
    // Non-blocking reading is not supported on TLS sockets right now
    // (it probably _can_ be done, we just didn't need it).
    assert(!nonblock);
    size_t bytes_received = 0;

    int status = mbedtls_ssl_read(&ssl_context, (unsigned char *)read_buffer, buffer_len);

    if (status <= 0) {
        if (net_context.timeout_happened) {
            return Error::Timeout;
        } else {
            return Error::Network;
        }
    }

    bytes_received = (size_t)status;

    return bytes_received;
}

bool tls::poll_readable(uint32_t) {
    // Not supported on tls connections (because not needed right now).
    assert(false);
    return true;
}

} // namespace connect_client
