#include "connection.hpp"

#include <cstring>

uint8_t Context::instances = 0;

/* No Host and Keep-Alive at this moment. */
static const char *HTTP_10 = "HTTP/1.0";

//! Supported function to buffer overflow vulnerability
#define LEN_ASSERT(len, p_len, fce)                   \
    if (len > p_len) {                                \
        lwsapi_error(fce ": End of line not found!"); \
        return ERR_VAL;                               \
    }

err_t Context::fill_request_buffer(const struct pbuf *p) {
    if (request_buffer == nullptr) {
        /* TODO: alloacate onle part for non parsed data
		   for example, if 500 bytes are parsed, allocat only
		   MAX_HTTP_REQUEST - 500*/
        request_buffer = reinterpret_cast<char *>(mem_malloc(MAX_HTTP_REQUEST));
        if (request_buffer == nullptr) {
            return ERR_MEM;
        }
    }
    for (auto it = p; it != nullptr; it = it->next) {
        uint16_t len = it->len;
        if (request_buffer_size + len > MAX_HTTP_REQUEST) {
            len = MAX_HTTP_REQUEST - request_buffer_size;
        }
        std::strncpy(request_buffer + request_buffer_size,
            reinterpret_cast<char *>(it->payload), len);
        request_buffer_size += len;
    }
    return ERR_OK;
}

size_t Context::find_eoh(const void *data, size_t length) {
    if (data == nullptr) {
        data = request_buffer;
        length = request_buffer_size;
    }

    // skip minimal request length
    for (size_t i = MINIMAL_REQUEST_LENGTH; i < (length - 3); i++) {
        if (!std::memcmp(memshift(data, i), CRLF CRLF, 4)) {
            return i + 3;
        }
    }
    return 0;
}

//! Parse request line : GET / HTTP/1.0
err_t Context::parse_request_line(const void *line, size_t length) {
    const void *end = memshift(std::memchr(line, ' ', length), -1);
    if (end == nullptr) {
        return ERR_VAL;
    }
    size_t len = memlen(line, end);
    // method is too long or line is too short
    if (len > METHOD_LENGTH || len == length) {
        return ERR_VAL;
    }

    std::strncpy(env.method, reinterpret_cast<const char *>(line), len);
    // skip the method
    line = memshift(end, 2);
    length = length - len - 1;

    end = memshift(std::memchr(line, ' ', length), -1);
    if (end == nullptr) {
        return ERR_VAL;
    }
    len = memlen(line, end);
    // uri is too long or line is too short
    if (len > URI_LENGTH || len == length) {
        return ERR_VAL;
    }

    std::strncpy(env.request_uri, reinterpret_cast<const char *>(line), len);
    // TODO: parse the HTTP version

    return ERR_OK;
}

//! Parse and set request headers from input buffer
err_t Context::parse_header(const void *line, size_t length) {
    const void *end = std::memchr(line, ':', length);
    if (end == nullptr) {
        return ERR_VAL;
    }

    const char *key = reinterpret_cast<const char *>(line);
    size_t klen = memlen(key, end) - 1; // skip ':'

    // skip ': '
    const char *val = chrshift(end, 2);
    size_t vlen = length - klen - 4; // ': \r\n'

    IHeader *header = request_header(key, klen, val, vlen);
    if (header != nullptr) {
        env.add_header(header);
        header->dbg();
    }
    return ERR_OK;
}

//! Parse and set request method and uri from input buffer
err_t Context::parse_request(const void *data, size_t length) {
    if (request_buffer != nullptr) {
        data = request_buffer;
        length = request_buffer_size;
    }

    const void *end = std::memchr(data, '\n', length);
    if (end == nullptr || chrshift(end, -1)[0] != '\r') { // end of line not found
        return ERR_VAL;
    }

    if (parse_request_line(data, memlen(data, end)) != ERR_OK) {
        return ERR_VAL;
    }

    length -= memlen(data, end);
    const void *start = memshift(end, 1);
    end = std::memchr(start, '\n', length);

    while (end != nullptr) {
        if (chrshift(end, -1)[0] != '\r') {
            return ERR_VAL;
        }

        // EOH detected
        if (memlen(start, end) == 2 && (!std::memcmp(start, CRLF, 2))) {
            return ERR_OK;
        }

        if (parse_header(start, memlen(start, end)) != ERR_OK) {
            return ERR_VAL;
        }
        length -= memlen(start, end);
        start = memshift(end, 1);
        end = std::memchr(start, '\n', length);
    }

    // EOH not found, that is error
    return ERR_VAL;
}

//! Process message.response to internal buffer
err_t Context::prepare_response() {
    // HTTP/1.0\ 200 OK\r\n\0
    size_t size = strlen(HTTP_10) + strlen(message.response) + 4;
    // TODO: check size
    if (buffer != nullptr) {
        mem_free(buffer); // repet call of this method could be possible
    }

    buffer = reinterpret_cast<char *>(mem_calloc(size, sizeof(char)));
    if (buffer == nullptr) {
        // no nullptr to response means try to prepare response in next call
        return ERR_MEM;
    }
    snprintf(buffer, size, "%s %s\r\n", HTTP_10, message.response);
    message.response = nullptr;
    return ERR_OK;
}

//! Process message.headers to internal buffer
err_t Context::prepare_header() {
    const IHeader *header = message.headers;
    size_t size = header->length() + 2; // + \r\n when it is last header
    // TODO: check size
    if (buffer != nullptr) {
        mem_free(buffer); // repet call of this method could be possible
    }

    buffer = reinterpret_cast<char *>(mem_calloc(size, sizeof(char)));
    if (buffer == nullptr) {
        return ERR_MEM; // no memmory yet
    }

    header->snprintf(buffer);
    if (header->next == nullptr) {
        buffer[size - 3] = '\r';
        buffer[size - 2] = '\n';
        buffer[size - 1] = '\0';
    }
    message.headers = header->next;
    return ERR_OK;
}
