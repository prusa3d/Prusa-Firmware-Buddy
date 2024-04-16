#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include "connection.hpp"
#include "types.h"

namespace http {

class ExtraHeader;
class WebSocket;

struct HeaderOut {
    const char *name = nullptr;
    std::variant<const char *, size_t> value = nullptr;
    std::optional<size_t> size_limit = std::nullopt;
};

class Request {
public:
    virtual ~Request() = default;
    virtual const char *url() const = 0;
    virtual http::ContentType content_type() const = 0;
    virtual Method method() const = 0;
    virtual const HeaderOut *extra_headers() const;
    virtual std::variant<size_t, Error> write_body_chunk(char *data, size_t size);
    virtual const char *connection() const;
};

class Response {
private:
    friend class HttpClient;
    // Will need to "steal" stuff from us.
    friend class WebSocket;
    static const constexpr size_t MAX_LEFTOVER = 64;
    std::array<uint8_t, MAX_LEFTOVER> body_leftover;
    size_t leftover_size;
    size_t content_length_rest;
    Connection *conn;

public:
    Response(Connection *conn, uint16_t status);
    Status status;
    ContentType content_type = ContentType::ApplicationOctetStream;
    std::optional<ContentEncryptionMode> content_encryption_mode;
    size_t content_length() const {
        return content_length_rest;
    }
    bool can_keep_alive = false;
    // Reads another part of body to the buffer.
    //
    // Either returns the number of bytes available or returns an error.
    // Returns 0 if no more data available.
    std::variant<size_t, Error> read_body(uint8_t *buffer, size_t buffer_size);

    // Reads everything from the response to the buffer.
    //
    // If it doesn't fit, it returns Error::ResponseTooLong.
    std::variant<size_t, Error> read_all(uint8_t *buffer, size_t buffer_size);
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
    std::variant<Response, Error> parse_response(Connection *conn, ExtraHeader *extra_resp_hdr);

public:
    HttpClient(ConnectionFactory &factory)
        : factory(factory) {}
    std::variant<Response, Error> send(Request &request, ExtraHeader *extra_resp_headers = nullptr);
};

} // namespace http
