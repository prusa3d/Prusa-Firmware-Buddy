#include "httpc.hpp"
#include "os_porting.hpp"
#include "resp_parser.h"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include "chunked.h"
#include "debug.h"

using automata::ExecutionControl;
using http::ConnectionHandling;
using http::ContentType;
using http::parser::ResponseParser;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::min;
using std::nullopt;
using std::optional;
using std::string_view;
using std::variant;

LOG_COMPONENT_DEF(httpc, LOG_SEVERITY_DEBUG);

namespace http {

namespace {

#define CHECKED(CALL)                       \
    if (auto err = CALL; err.has_value()) { \
        return *err;                        \
    }

    class OutBuffer {
    private:
        static constexpr const size_t buffer_size = 256;
        char buffer[buffer_size] = {};
        size_t used = 0;
        Connection *conn;

    public:
        OutBuffer(Connection *conn)
            : conn(conn) {}
        optional<Error> flush() {
            size_t total_sent = 0;
            uint32_t epoch_time = ticks_ms();

            while (total_sent < used) {
                std::variant<size_t, Error> ret = conn->tx(reinterpret_cast<const uint8_t *>(buffer + total_sent), used - total_sent);

                if (holds_alternative<Error>(ret)) {
                    return get<Error>(ret);
                }

                size_t written = get<size_t>(ret);
                if (written > 0) {
                    epoch_time = ticks_ms();
                }

                total_sent += written;

                // FIXME: Wrap around check?
                // FIXME: Conceptually, how do we handle timeouts? This is timeout
                // for a single write. We may write multiple times per request.
                // Also, what is the timeout in the tx?
                if ((epoch_time + SOCKET_TIMEOUT_SEC * 1000) < ticks_ms()) {
                    break;
                }
            }

            if (total_sent != used) {
                return Error::Timeout;
            }

            used = 0;

            return std::nullopt;
        }
        optional<Error> write_fmt(const char *format, ...) {
            va_list args;

            size_t rest = sizeof(buffer) - used;
            va_start(args, format);
            size_t attempted = vsnprintf(buffer + used, rest, format, args);
            va_end(args);

            if (attempted < rest) { // Note: if equal, the last byte written was \0 and was truncated
                used += attempted;
                return nullopt;
            }

            // Not enough space in the current buffer. Try sending it out and using a full space.
            CHECKED(flush());
            assert(used == 0);

            // Won't fit even into an empty one.
            if (attempted >= sizeof(buffer)) {
                return Error::InternalError;
            }

            va_start(args, format);
            // Results in the same value...
            vsnprintf(buffer + used, rest, format, args);
            va_end(args);

            used += attempted;

            return nullopt;
        }
        optional<Error> header(const char *name, const char *value, std::optional<size_t> limit) {
            if (limit.has_value()) {
                return write_fmt("%s: %.*s\r\n", name, *limit, value);
            } else {
                return write_fmt("%s: %s\r\n", name, value);
            }
        }
        template <class R>
        void chunk(R renderer) {
            assert(used == 0); // Not yet supported
            char *buffer = this->buffer;
            used = http::render_chunk(ConnectionHandling::ChunkedKeep, reinterpret_cast<uint8_t *>(buffer), sizeof this->buffer, renderer);
        }
    };

}

const HeaderOut *Request::extra_headers() const {
    return nullptr;
}

variant<size_t, Error> Request::write_body_chunk(char *, size_t) {
    return static_cast<size_t>(0);
}

Response::Response(Connection *conn, uint16_t status)
    : conn(conn)
    , status(static_cast<http::Status>(status)) {}

variant<size_t, Error> Response::read_body(uint8_t *buffer, size_t size) {
    size_t pos = 0;
    size = min(size, content_length());

    // TODO: Dealing with chunked and connection-closed bodies
    while (pos < size) {
        size_t available = size - pos;
        uint8_t *write_pos = buffer + pos;
        if (leftover_size > 0) {
            size_t chunk = min(leftover_size, available);
            memcpy(write_pos, body_leftover.data(), chunk);
            leftover_size -= chunk;
            memmove(body_leftover.data(), body_leftover.data() + chunk, leftover_size);
            pos += chunk;
        } else {
            auto read_resp = conn->rx(write_pos, available);
            if (holds_alternative<Error>(read_resp)) {
                return get<Error>(read_resp);
            }

            size_t added = get<size_t>(read_resp);
            if (added == 0) {
                // EOF
                break;
            }

            assert(added <= available);
            pos += added;
        }
    }

    content_length_rest -= pos;
    return pos;
}

optional<Error> HttpClient::send_request(const char *host, Connection *conn, Request &request) {
    OutBuffer buffer(conn);

    const Method method = request.method();

    CHECKED(buffer.write_fmt("%s %s HTTP/1.1\r\n", to_str(method), request.url()));
    CHECKED(buffer.header("Host", host, nullopt));
    CHECKED(buffer.header("Connection", "keep-alive", nullopt));
    if (has_body(method)) {
        CHECKED(buffer.header("Transfer-Encoding", "chunked", nullopt));
        CHECKED(buffer.header("Content-Type", to_str(request.content_type()), nullopt));
    }

    static const constexpr HeaderOut term = { nullptr, nullptr, nullopt };
    for (const HeaderOut *extra_hdrs = request.extra_headers() ?: &term; extra_hdrs->name; extra_hdrs++) {
        CHECKED(buffer.header(extra_hdrs->name, extra_hdrs->value, extra_hdrs->size_limit));
    }

    CHECKED(buffer.write_fmt("\r\n"));

    optional<Error> err_out = nullopt;

    if (has_body(method)) {
        bool has_more = true;
        while (has_more) {
            // Currently, the body generation doesn't handle split/small buffer. So
            // we flush it first to give it the full body.
            CHECKED(buffer.flush());
            buffer.chunk([&](uint8_t *buffer, size_t buffer_size) -> optional<size_t> {
                const auto result = request.write_body_chunk(reinterpret_cast<char *>(buffer), buffer_size);
                if (holds_alternative<size_t>(result)) {
                    size_t written = get<size_t>(result);
                    if (written == 0) {
                        has_more = false;
                    }
                    return written;
                } else {
                    has_more = false;
                    err_out = get<Error>(result);
                    return nullopt;
                }
            });
        }
    }

    CHECKED(buffer.flush());

    return err_out;
}

variant<Response, Error> HttpClient::parse_response(Connection *conn) {
    ResponseParser parser;

    uint8_t buffer[Response::MAX_LEFTOVER];

    // TODO: Timeouts
    for (;;) {
        auto read = conn->rx(buffer, sizeof buffer);
        if (holds_alternative<Error>(read)) {
            return get<Error>(read);
        }
        size_t available = get<size_t>(read);
        if (available == 0) {
            // Closed connection.
            return Error::Network;
        }
        uint8_t *b = buffer;
        const auto [parse_result, consumed] = parser.consume(string_view(reinterpret_cast<const char *>(b), available));

        if (!parser.done && parse_result == ExecutionControl::NoTransition) {
            return Error::Parse;
        }

        if (parser.done) {
            Response response(conn, parser.status_code);
            size_t rest = available - consumed;
            memcpy(response.body_leftover.data(), buffer + consumed, rest);
            // TODO: Do we want to somehow handle missing content length? For now, this is good enough.
            response.content_length_rest = parser.content_length.value_or(0);
            response.leftover_size = rest;
            response.content_type = parser.content_type;
            response.command_id = parser.command_id;
            if (parser.keep_alive.has_value()) {
                response.can_keep_alive = *parser.keep_alive;
            } else {
                response.can_keep_alive = (parser.version_major == 1) && (parser.version_minor >= 1);
            }
            return std::move(response);
        }
    }
}

variant<Response, Error> HttpClient::send(Request &request) {
    auto conn_raw = factory.connection();
    if (holds_alternative<Error>(conn_raw)) {
        return get<Error>(conn_raw);
    }
    auto conn = get<Connection *>(conn_raw);
    assert(conn != nullptr);
    const char *host = factory.host();

    if (auto error = send_request(host, conn, request); error.has_value()) {
        // Note: the current architecture doesn't go well with early results
        // from server. If a server sends a response early on (after headers or
        // mid-body) and closes the connection hard way, we get connection
        // reset which prevents more writing into it. But it also, at that
        // point, prevents reading the response.
        factory.invalidate();
        return *error;
    }

    return HttpClient::parse_response(conn);
}

}
