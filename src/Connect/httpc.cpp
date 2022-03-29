#include "httpc.hpp"
#include "os_porting.hpp"

#include <log.h>
#include <cstring>
#include <cstdlib>
#include <debug.h>

namespace con {

constexpr size_t header_buffer_size = 170;

LOG_COMPONENT_DEF(httpc, LOG_SEVERITY_DEBUG);

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
