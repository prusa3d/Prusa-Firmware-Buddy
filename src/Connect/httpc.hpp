#pragma once

#include "tls/tls.hpp"

#include "connection.hpp"
#include <cstdint>
#include <optional>
#include <variant>

namespace con {

/* A very basic HTTP client implementation for Connect. The implementation
   only expects smaller data sizes. If in case the received data is larger
   than the buffer limits the data is ignored.

   * Header and Body are send to server seperately.
   * Header uses buffer in the stack memory.
   * Caller of the client provides buffer for Body.
   * Receive function must be provided with a buffer for HTTP body.
   * Receive function internally uses buffer in stack for both header and body
   * The received body is copied to user's buffer on success
   * Implementation expects the whole data (Header + Body) to receive before
        the data is handed over to the caller.
*/

enum class REQUEST_TYPE {
    TELEMETRY,
    SEND_INFO
};

class http_client {

private:
    char *host;
    uint16_t port;
    Connection *con;
    bool connected = false;
    size_t content_length;

public:
    http_client(char *host, uint16_t port, class Connection *con);
    bool is_connected();
    std::optional<Error> send_data(uint8_t *buffer, size_t data_len);
    std::optional<Error> send_header(REQUEST_TYPE type, char *fingerprint, char *token, size_t content_length);
    std::optional<Error> send_body(uint8_t *body, size_t buffer_size);
};

}
