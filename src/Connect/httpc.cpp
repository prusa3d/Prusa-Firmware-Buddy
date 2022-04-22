#include "httpc.hpp"
#include "os_porting.hpp"

#include <log.h>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <debug.h>

#include <http/chunked.h>

using http::ConnectionHandling;
using std::get;
using std::holds_alternative;
using std::nullopt;
using std::optional;
using std::variant;

namespace con {

namespace {

    constexpr size_t header_buffer_size = 170;

    const char *to_str(Method method) {
        // TODO: Implement other methods
        return "POST";
    }

    const char *to_str(ContentType content_type) {
        // TODO: Implement other content types
        return "application/json";
    }

    bool has_out_body(Method method) {
        // TODO: Implement other methods
        return true;
    }

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
                return Error::BUFFER_OVERFLOW_ERROR;
            }

            va_start(args, format);
            // Results in the same value...
            vsnprintf(buffer + used, rest, format, args);
            va_end(args);

            used += attempted;

            return nullopt;
        }
        optional<Error> header(const char *name, const char *value) {
            return write_fmt("%s: %s\r\n", name, value);
        }
        template <class R>
        void chunk(R renderer) {
            assert(used == 0); // Not yet supported
            char *buffer = this->buffer;
            used = http::render_chunk(ConnectionHandling::ChunkedKeep, reinterpret_cast<uint8_t *>(buffer), sizeof buffer, renderer);
        }
    };

}

LOG_COMPONENT_DEF(httpc, LOG_SEVERITY_DEBUG);

HeaderOut *Request::extra_headers() const {
    return nullptr;
}

size_t Request::write_body_chunk(char *, size_t) {
    return 0;
}

optional<Error> HttpClient::send_request(const char *host, Connection *conn, Request &request) {
    OutBuffer buffer(conn);

    const Method method = request.method();

    CHECKED(buffer.write_fmt("%s %s HTTP/1.1\r\n", to_str(method), request.url()));
    CHECKED(buffer.header("Host", host));
    // TODO: Once we _read_ the response, we want Keep-Alive here
    CHECKED(buffer.header("Connection", "Close"));
    if (has_out_body(method)) {
        CHECKED(buffer.header("Transfer-Encoding", "chunked"));
        CHECKED(buffer.header("Content-Type", to_str(request.content_type())));
    }

    static const constexpr HeaderOut term = { "", "" };
    for (const HeaderOut *extra_hdrs = request.extra_headers() ?: &term; extra_hdrs->name; extra_hdrs++) {
        CHECKED(buffer.header(extra_hdrs->name, extra_hdrs->value));
    }

    CHECKED(buffer.write_fmt("\r\n"));

    if (has_out_body(method)) {
        bool has_more = true;
        while (has_more) {
            // Currently, the body generation doesn't handle split/small buffer. So
            // we flush it first to give it the full body.
            CHECKED(buffer.flush());
            buffer.chunk([&](uint8_t *buffer, size_t buffer_size) -> size_t {
                const size_t written = request.write_body_chunk(reinterpret_cast<char *>(buffer), buffer_size);
                if (written == 0) {
                    has_more = false;
                }
                return written;
            });
        }
    }

    CHECKED(buffer.flush());

    return nullopt;
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
        factory.invalidate();
        return *error;
    }

    // TODO: Read the response
    return Response {};
}

http_client::http_client(char *host, uint16_t port, class Connection *con)
    : host { host }
    , port { port }
    , con { con } {
    std::optional<Error> ret = con->connection(host, port);
    if (!ret.has_value())
        connected = true;
}

bool http_client::is_connected() {
    return connected;
}

std::optional<Error> http_client::send_data(uint8_t *buffer, size_t data_len) {
    if (NULL == buffer)
        return Error::INVALID_PARAMETER_ERROR;

    size_t total_sent = 0;
    uint32_t epoch_time = ticks_ms();

    while (total_sent < data_len) {

        std::variant<size_t, Error> ret = con->tx(buffer + total_sent, data_len - total_sent);

        if (std::holds_alternative<Error>(ret))
            return Error::WRITE_ERROR;

        if (std::get<size_t>(ret) > 0)
            epoch_time = ticks_ms();

        total_sent += std::get<size_t>(ret);

        if ((epoch_time + SOCKET_TIMEOUT_SEC) < ticks_ms())
            break;
    }

    if (total_sent != data_len)
        return Error::WRITE_ERROR;

    return std::nullopt;
}

std::optional<Error> http_client::send_header(REQUEST_TYPE type, char *fingerprint, char *token, size_t length) {
    if ((NULL == fingerprint) || (NULL == token))
        return Error::INVALID_PARAMETER_ERROR;
    if (!strlen(fingerprint) || !strlen(token))
        return Error::INVALID_PARAMETER_ERROR;

    content_length = length;

    char hbuffer[header_buffer_size];
    size_t header_length = 0;
    size_t bytes_written = 0;
    size_t buffer_left = header_buffer_size;

    if (REQUEST_TYPE::TELEMETRY == type) {
        bytes_written = snprintf(hbuffer, buffer_left, "POST /p/telemetry HTTP/1.0\r\n");
    } else if (REQUEST_TYPE::SEND_INFO == type) {
        bytes_written = snprintf(hbuffer, buffer_left, "POST /p/events HTTP/1.0\r\n");
    } else {
        return Error::INVALID_PARAMETER_ERROR;
    }

    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    bytes_written = snprintf(hbuffer + header_length, buffer_left, "Host: %s\r\n", host);
    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    bytes_written = snprintf(hbuffer + header_length, buffer_left, "Token: %s\r\n", token);
    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    bytes_written = snprintf(hbuffer + header_length, buffer_left, "Fingerprint: %s\r\n", fingerprint);
    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    bytes_written = snprintf(hbuffer + header_length, buffer_left, "Content-Length: %zu\r\n", content_length);
    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    bytes_written = snprintf(hbuffer + header_length, buffer_left, "Content-Type: application/json\r\n");
    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    bytes_written = snprintf(hbuffer + header_length, buffer_left, "\r\n");
    if ((bytes_written >= buffer_left) || bytes_written < 0)
        return Error::ERROR;
    header_length += bytes_written;
    buffer_left = header_buffer_size - header_length;

    CONNECT_DEBUG("--- header: %zu bytes\n%s\n", header_length, hbuffer);
    log_debug(httpc, "\n--- header:\n%s\n", hbuffer);

    return send_data((uint8_t *)hbuffer, header_length);
}

std::optional<Error> http_client::send_body(uint8_t *body, size_t buffer_size) {
    CONNECT_DEBUG("\n--- body:\n%s\n", body);
    log_debug(httpc, "\n--- body:\n%s\n", body);

    if (buffer_size < content_length)
        return Error::INVALID_PARAMETER_ERROR;

    return send_data(body, content_length);
}

}
