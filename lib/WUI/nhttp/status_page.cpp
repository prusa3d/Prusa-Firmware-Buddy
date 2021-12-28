#include "status_page.h"
#include "handler.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace nhttp::handler {

namespace {

    const char *authenticate_hdrs[] = {
        "WWW-Authenticate: ApiKey realm=\"Printer API\"\r\n",
        nullptr,
    };

    struct StatusText {
        Status status;
        const char *text;
        const char **extra_hdrs = nullptr;
    } texts[] = {
        { Status::Unknown, "???" },
        { Status::Ok, "OK" },
        { Status::NoContent, "No Content" },
        { Status::BadRequest, "Bad Request" },
        { Status::Unauthorized, "Unauthorized", authenticate_hdrs },
        { Status::NotFound, "Not Found" },
        { Status::MethodNotAllowerd, "Method Not Allowed" },
        { Status::Conflict, "Conflict" },
        { Status::UnsupportedMediaType, "Unsupported Media Type" },
        { Status::IMATeaPot, "I'm a Teapot Printer" },
        { Status::UriTooLong, "URI Too Long" },
        { Status::UnprocessableEntity, "Unprocessable Entity" },
        { Status::RequestHeaderFieldsTooLarge, "Request Header Fields Too Large" },
        { Status::InternalServerError, "Infernal Server Error" },
        { Status::NotImplemented, "Not Implemented" },
        { Status::ServiceTemporarilyUnavailable, "Service Temporarily Unavailable" },
        { Status::InsufficientStorage, "Insufficient Storage" },
    };

    StatusText *find_status(Status status) {
        for (StatusText *t = texts; t < (texts + sizeof(texts) / sizeof(*texts)); t++) {
            if (t->status == status) {
                return t;
            }
        }

        return texts;
    }

    constexpr const size_t content_buffer_len = 128;

    const char *content_type_string(ContentType content_type) {
        switch (content_type) {
        case ContentType::TextPlain:
            return "text/plain";
        case ContentType::TextHtml:
            return "text/html; charset=utf-8";
        case ContentType::TextCss:
            return "text/css";
        case ContentType::ApplicationJavascript:
            return "application/javascript";
        case ContentType::ApplicationJson:
            return "application/json";
        case ContentType::ApplicationOctetStream:
            return "application/octet-stream";
        default:
            assert(0);
            return "application/octet-stream";
        }
    }

}

size_t write_headers(uint8_t *buffer, size_t buffer_len, Status status, ContentType content_type, ConnectionHandling handling, std::optional<uint64_t> content_length, const char **extra_hdrs) {
    // Always leave space for the body-newline separator
    assert(buffer_len > 2);
    buffer_len -= 2;

    char *buf = reinterpret_cast<char *>(buffer);
    StatusText *text = find_status(status);
    size_t pos = snprintf(buf, buffer_len,
        "HTTP/1.1 %u %s\r\n"
        "Content-Type: %s\r\n"
        "Connection: %s\r\n",
        static_cast<unsigned>(status),
        text->text,
        content_type_string(content_type),
        handling == ConnectionHandling::Close ? "close" : "keep-alive");
    // snprintf likes to return how much it _would_ print were there enough space.
    pos = std::min(buffer_len, pos);
    if (content_length.has_value() && pos < buffer_len) {
        pos += snprintf(buf + pos, buffer_len - pos, "Content-Length: %" PRIu64 "\r\n", *content_length);
        pos = std::min(buffer_len, pos);
    }
    while (extra_hdrs && *extra_hdrs) {
        size_t copy = std::min(buffer_len - pos, strlen(*extra_hdrs));
        memcpy(buf + pos, *extra_hdrs, copy);
        pos += copy;
    }

    buffer_len += 2;
    memcpy(buf + pos, "\r\n", 2);
    pos += 2;
    return pos;
}

Step StatusPage::step(std::string_view, bool, uint8_t *output, size_t output_size) {
    /*
     * Note: we assume the buffers has reasonable size and our payload fits. We
     * won't do out-of-range access if not, but the response would be
     * truncated.
     */
    assert(output && output_size > 0);

    const StatusText *text = find_status(status);

    char content_buffer[128];
    snprintf(content_buffer, sizeof(content_buffer), "<html><body><h1>%u: %s</h1><p>%s", static_cast<unsigned>(status), text->text, extra_content);

    ConnectionHandling handling = can_keep_alive ? ConnectionHandling::ContentLengthKeep : ConnectionHandling::Close;

    size_t used_up = write_headers(output, output_size, status, ContentType::TextHtml, handling, strlen(content_buffer), text->extra_hdrs);
    size_t rest = output_size - used_up;
    size_t write = std::min(strlen(content_buffer), rest);
    // If we use up the whole buffer, there's no \0 at the end. We are fine
    // with that, we work with byte-arrays with lenghts here.
    strncpy(reinterpret_cast<char *>(output + used_up), content_buffer, write);

    return Step { 0, used_up + write, Terminating::for_handling(handling) };
}

}
