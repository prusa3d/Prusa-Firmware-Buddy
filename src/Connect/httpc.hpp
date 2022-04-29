#pragma once

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
    virtual const HeaderOut *extra_headers() const;
    virtual std::variant<size_t, Error> write_body_chunk(char *data, size_t size);
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

}
