#include "tls.hpp"
#include "certificate.h"
#include <string.h>
#include <stdbool.h>

namespace con {

tls::tls() {
    mbedtls_net_init(&net_context);
    mbedtls_ssl_init(&ssl_context);
    mbedtls_ssl_config_init(&ssl_config);
}

tls::~tls() {
    mbedtls_ssl_close_notify(&ssl_context);
    mbedtls_net_free(&net_context);
    mbedtls_ssl_free(&ssl_context);
    mbedtls_ssl_config_free(&ssl_config);
}

std::optional<Error> tls::connect(char *host, uint16_t port) {

    mbedtls_x509_crt x509_certificate;
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context drbg_context;
    int status;

    mbedtls_x509_crt_init(&x509_certificate);
    mbedtls_entropy_init(&entropy_context);
    mbedtls_ctr_drbg_init(&drbg_context);

    if ((status = mbedtls_ctr_drbg_seed(&drbg_context, mbedtls_entropy_func, &entropy_context, NULL, 0)) != 0) {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
        return Error::CONNECTION_ERROR;
    }

    mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &drbg_context);
    if ((status = mbedtls_x509_crt_parse_der_nocopy(&x509_certificate, (const unsigned char *)ca_cert_der, ca_cert_der_len))
        != 0) {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
        return Error::CONNECTION_ERROR;
    }

    mbedtls_ssl_conf_ca_chain(&ssl_config, &x509_certificate, NULL);

    if ((status = mbedtls_ssl_config_defaults(&ssl_config,
             MBEDTLS_SSL_IS_CLIENT,
             MBEDTLS_SSL_TRANSPORT_STREAM,
             MBEDTLS_SSL_PRESET_DEFAULT))
        != 0) {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
        return Error::CONNECTION_ERROR;
    }

    // Only use TLS 1.2
    mbedtls_ssl_conf_max_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    // Strictly ensure that certificates are signed by the CA
    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);

    // set cipher suite to use
    static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, 0 };
    mbedtls_ssl_conf_ciphersuites(&ssl_config, tls_cipher_suites);

    mbedtls_ssl_set_hostname(&ssl_context, (const char *)host);

    if ((status = mbedtls_ssl_setup(&ssl_context, &ssl_config)) != 0) {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
        return Error::CONNECTION_ERROR;
    }

    mbedtls_ssl_set_bio(&ssl_context, &net_context, mbedtls_net_send, mbedtls_net_recv, NULL);

    constexpr size_t str_len = 6;
    char port_as_str[str_len] = {};
    snprintf(port_as_str, str_len, "%hu", port);

    if ((status = mbedtls_net_connect(&net_context, (const char *)host, port_as_str, MBEDTLS_NET_PROTO_TCP)) != 0) {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
        return Error::CONNECTION_ERROR;
    }

    while ((status = mbedtls_ssl_handshake(&ssl_context)) != 0) {
        if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_ctr_drbg_free(&drbg_context);
            mbedtls_entropy_free(&entropy_context);
            mbedtls_x509_crt_free(&x509_certificate);
            return Error::CONNECTION_ERROR;
        }
    }

    if ((status = mbedtls_ssl_get_verify_result(&ssl_context)) != 0) {
        mbedtls_ctr_drbg_free(&drbg_context);
        mbedtls_entropy_free(&entropy_context);
        mbedtls_x509_crt_free(&x509_certificate);
        return Error::TLS_CERT_ERROR;
    }

    mbedtls_ctr_drbg_free(&drbg_context);
    mbedtls_entropy_free(&entropy_context);
    mbedtls_x509_crt_free(&x509_certificate);
    return Error::OK;
}

std::variant<size_t, Error> tls::write(uint8_t *send_buffer, size_t data_len) {
    size_t bytes_sent = 0;

    int status = mbedtls_ssl_write(&ssl_context, (const unsigned char *)send_buffer, data_len);

    if (status <= 0) {
        return Error::WRITE_ERROR;
    }

    bytes_sent = (size_t)status;
    return bytes_sent;
}

std::variant<size_t, Error> tls::read(uint8_t *read_buffer, size_t buffer_len) {
    size_t bytes_received = 0;

    int status = mbedtls_ssl_read(&ssl_context, (unsigned char *)read_buffer, buffer_len);

    if (status <= 0) {
        return Error::READ_ERROR;
    }

    bytes_received = (size_t)status;

    return bytes_received;
}

}
