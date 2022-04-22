#pragma once

#include "tls/tls.hpp"

#include "connection.hpp"
#include <cstdint>
#include <optional>
#include <variant>

namespace con {

// TODO: Bring in...
enum class ContentType {
    ApplicationJson
};

enum class Method {
    Post
};

struct HeaderOut {
    const char *name;
    const char *value;
};

class Request {
public:
    virtual ~Request() = default;
    virtual const char *url() const = 0;
    virtual ContentType content_type() const = 0;
    virtual Method method() const = 0;
    virtual HeaderOut *extra_headers() const;
    virtual size_t write_body_chunk(char *data, size_t size);
};

class Response {
    // FIXME: Empty for now
};

class ConnectionFactory {
public:
    virtual std::variant<Connection *, Error> connection() = 0;
    virtual const char *host() = 0;
    virtual void invalidate() = 0;
    virtual ~ConnectionFactory() = default;
};

class HttpClient {
private:
    ConnectionFactory &factory;
    std::optional<Error> send_request(const char *host, Connection *conn, Request &request);

public:
    HttpClient(ConnectionFactory &factory)
        : factory(factory) {}
    std::variant<Response, Error> send(Request &request);
};

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
