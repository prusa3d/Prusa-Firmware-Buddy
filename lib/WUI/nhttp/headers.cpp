#include "headers.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <inttypes.h> // PRIu* macros (not available in <cinttypes>)
#include <mbedtls/sha256.h>

using http::ConnectionHandling;
using http::ContentType;
using http::Status;

namespace nhttp {

namespace {

    const constexpr StatusText texts[] = {
        { Status::UnknownStatus, "???" },
        { Status::Ok, "OK" },
        { Status::Created, "Created" },
        { Status::Accepted, "Accepted" },
        { Status::NoContent, "No Content" },
        { Status::PartialContent, "Partial Content" },
        { Status::NotModified, "Not Modified" },
        { Status::BadRequest, "Bad Request" },
        { Status::Forbidden, "Forbidden" },
        { Status::Unauthorized, "Unauthorized" },
        { Status::NotFound, "Not Found" },
        { Status::MethodNotAllowed, "Method Not Allowed" },
        { Status::RequestTimeout, "Request Timeout" },
        { Status::Conflict, "Conflict" },
        { Status::LengthRequired, "Length Required" },
        { Status::UnsupportedMediaType, "Unsupported Media Type" },
        { Status::IMATeaPot, "I'm a Teapot Printer" },
        { Status::PayloadTooLarge, "Payload Too Large" },
        { Status::UriTooLong, "URI Too Long" },
        { Status::UnprocessableEntity, "Unprocessable Entity" },
        { Status::TooManyRequests, "Too Many Requests" },
        { Status::RequestHeaderFieldsTooLarge, "Request Header Fields Too Large" },
        { Status::InternalServerError, "Infernal Server Error" },
        { Status::NotImplemented, "Not Implemented" },
        { Status::ServiceTemporarilyUnavailable, "Service Temporarily Unavailable" },
        { Status::GatewayTimeout, "Gateway Timeout" },
        { Status::InsufficientStorage, "Insufficient Storage" },
    };

    constexpr const size_t content_buffer_len = 128;
} // namespace

const StatusText &StatusText::find(Status status) {
    for (const StatusText *t = texts; t < (texts + sizeof(texts) / sizeof(*texts)); t++) {
        if (t->status == status) {
            return *t;
        }
    }

    return *texts;
}

size_t write_headers(uint8_t *buffer, size_t buffer_len, Status status, ContentType content_type, ConnectionHandling handling, std::optional<uint64_t> content_length, std::optional<uint32_t> etag, const char *const *extra_hdrs) {
    // Always leave space for the body-newline separator
    assert(buffer_len > 2);
    buffer_len -= 2;

    char *buf = reinterpret_cast<char *>(buffer);
    const StatusText &text = StatusText::find(status);
    size_t pos = snprintf(buf, buffer_len,
        "HTTP/1.1 %u %s\r\n"
        "Content-Type: %s\r\n"
        "Connection: %s\r\n",
        static_cast<unsigned>(status),
        text.text,
        to_str(content_type),
        handling == ConnectionHandling::Close ? "close" : "keep-alive");
    // snprintf likes to return how much it _would_ print were there enough space.
    pos = std::min(buffer_len, pos);
    if (content_length.has_value() && pos < buffer_len) {
        pos += snprintf(buf + pos, buffer_len - pos, "Content-Length: %" PRIu64 "\r\n", *content_length);
        pos = std::min(buffer_len, pos);
    }
    if (handling == ConnectionHandling::ChunkedKeep && pos < buffer_len) {
        pos += snprintf(buf + pos, buffer_len - pos, "Transfer-Encoding: chunked\r\n");
        pos = std::min(buffer_len, pos);
    }
    if (etag.has_value() && pos < buffer_len) {
        pos += snprintf(buf + pos, buffer_len - pos, "ETag: \"%" PRIu32 "\"\r\n", *etag);
        pos = std::min(buffer_len, pos);
    }
    for (; extra_hdrs && *extra_hdrs; extra_hdrs++) {
        size_t copy = std::min(buffer_len - pos, strlen(*extra_hdrs));
        memcpy(buf + pos, *extra_hdrs, copy);
        pos += copy;
    }

    // That 2 fits, reserved at the top of the function.
    memcpy(buf + pos, "\r\n", 2);
    pos += 2;
    return pos;
}

ContentType guess_content_by_ext(const char *filename) {
    const char *last_dot = rindex(filename, '.');
    if (!last_dot) {
        return ContentType::ApplicationOctetStream;
    }

    const char *ext = last_dot + 1;

    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) {
        return ContentType::TextHtml;
    } else if (strcasecmp(ext, "css") == 0) {
        return ContentType::TextCss;
    } else if (strcasecmp(ext, "js") == 0) {
        return ContentType::ApplicationJavascript;
    } else if (strcasecmp(ext, "ico") == 0) {
        return ContentType::ImageIco;
    } else if (strcasecmp(ext, "png") == 0) {
        return ContentType::ImagePng;
    } else if (strcasecmp(ext, "svg") == 0) {
        return ContentType::ImageSvg;
    }

    return ContentType::ApplicationOctetStream;
}

uint32_t compute_etag(const struct stat &stat) {
    /*
     * We need something that hopefully changes whenever someone replaces a
     * file with a different file but the same name. Hashing a bit of metadata
     * seems like a good candidate.
     */
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, false);
    mbedtls_sha256_update_ret(&ctx, (const uint8_t *)&stat.st_ino, sizeof stat.st_ino);
    mbedtls_sha256_update_ret(&ctx, (const uint8_t *)&stat.st_size, sizeof stat.st_size);
    mbedtls_sha256_update_ret(&ctx, (const uint8_t *)&stat.st_mtime, sizeof stat.st_mtime);
    mbedtls_sha256_update_ret(&ctx, (const uint8_t *)&stat.st_ctime, sizeof stat.st_ctime);
    uint8_t hash[32];
    mbedtls_sha256_finish_ret(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    /*
     * Use memcpy to deal with alignment issues and such.
     *
     * We don't care about endians or such, the result will be the same each
     * time and we don't care what _exactly_ it'll be.
     */
    uint32_t result;
    static_assert(sizeof result <= sizeof hash);
    memcpy((uint8_t *)&result, hash, sizeof result);

    return result;
}

} // namespace nhttp
