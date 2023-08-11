/**
 ******************************************************************************
 * File Name       : net_sockets.c.h
 * Description     : TCP/IP or UDP/IP networking functions implementation based
                    on LwIP API see the file "mbedTLS/library/net_socket_template.c"
                    for the standard implmentation
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

#include <string.h>
#include <stdint.h>
#include <sys/socket.h>

#include <lwip/netdb.h>
#include <lwip/sockets.h>

#include "net_sockets.hpp"

using http::Error;
using std::get;
using std::get_if;
using std::variant;

namespace {

int convert_result(variant<size_t, Error> result, int timeout_error) {
    if (auto *amt = get_if<size_t>(&result); amt != nullptr) {
        return *amt;
    } else {
        Error err = get<Error>(result);
        switch (err) {
        case Error::Network:
            return MBEDTLS_ERR_NET_CONN_RESET;
        case Error::Timeout:
            return timeout_error;
        default:
            return MBEDTLS_ERR_NET_RECV_FAILED;
        }
    }
}

} // namespace

mbedtls_net_context::mbedtls_net_context(uint8_t timeout_s)
    : plain_conn(timeout_s)
    , timeout_happened(false) {}

void mbedtls_net_init(mbedtls_net_context *) {
    // We have a real constructor there, but we still shall have this function.
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int /* proto */) {
    // We assume the string is correct number.
    //
    // Why would anyone pass port as a string anyway? :-(
    uint16_t port_int = atoi(port);
    auto error = ctx->plain_conn.connection(host, port_int);
    if (error.has_value()) {
        switch (*error) {
        case Error::Dns:
            return MBEDTLS_ERR_NET_UNKNOWN_HOST;
        default:
            // Most errors are either impossible here, or mbedtls has no specific way to signal.
            return MBEDTLS_ERR_NET_CONNECT_FAILED;
        }
    } else {
        return 0;
    }
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
    mbedtls_net_context *context = reinterpret_cast<mbedtls_net_context *>(ctx);
    // Note: We don't support/use nonblock mode on TLS sockets, which makes this code a bit easier.
    auto result = context->plain_conn.rx(buf, len, false);
    auto converted = convert_result(result, MBEDTLS_ERR_SSL_WANT_READ);

    if (converted == MBEDTLS_ERR_SSL_WANT_READ) {
        // Note: read in the connection factory (tls.cpp), we use this to smuggle the info through several layers up.
        context->timeout_happened = true;
    }

    return converted;
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len) {
    mbedtls_net_context *context = reinterpret_cast<mbedtls_net_context *>(ctx);
    auto result = context->plain_conn.tx(buf, len);
    auto converted = convert_result(result, MBEDTLS_ERR_SSL_WANT_WRITE);

    if (converted == MBEDTLS_ERR_SSL_WANT_WRITE) {
        // Note: read in the connection factory (tls.cpp), we use this to smuggle the info through several layers up.
        context->timeout_happened = true;
    }

    return converted;
}

void mbedtls_net_free(mbedtls_net_context *) {
    // Handled by the actual destructor on ctx.
}
