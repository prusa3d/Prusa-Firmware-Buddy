#pragma once

#include <array>
#include <cstdlib>
#include <cassert>
#include <cstdint>

namespace http {

enum Method {
    Head,
    Get,
    Post,
    Put,
    Delete,
    UnknownMethod,
};

constexpr const char *to_str(Method method) {
    switch (method) {
    case Head:
        return "HEAD";
    case Get:
        return "GET";
    case Post:
        return "POST";
    case Put:
        return "PUT";
    case Delete:
        return "DELETE";
    default:
        assert(0);
    case UnknownMethod:
        return "UNKNOWNMETHOD";
    }
}

constexpr bool has_body(Method method) {
    switch (method) {
    case Post:
    case Put:
        return true;
    case Get:
    case Head:
    case Delete:
        return false;
    default:
    case UnknownMethod:
        assert(0);
        return false;
    }
}

enum class ContentType {
    TextPlain,
    TextHtml,
    TextCss,
    TextGcode,
    ImageIco,
    ImagePng,
    ImageSvg,
    ApplicationJavascript,
    ApplicationJson,
    ApplicationOctetStream,
};

enum class ContentEncryptionMode {
    AES_CBC,
    AES_CTR,
};

constexpr const char *to_str(ContentType content_type) {
    switch (content_type) {
    case ContentType::TextPlain:
        return "text/plain";
    case ContentType::TextHtml:
        return "text/html; charset=utf-8";
    case ContentType::TextCss:
        return "text/css";
    case ContentType::TextGcode:
        return "text/g-code";
    case ContentType::ImageIco:
        return "image/vnd.microsoft.icon";
    case ContentType::ImagePng:
        return "image/png";
    case ContentType::ImageSvg:
        return "image/svg+xml";
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

enum class ConnectionHandling {
    Close,
    ContentLengthKeep,
    ChunkedKeep,
};

// Note: Not exhaustive! Therefore not enum class, just enum.
enum Status {
    UnknownStatus = 0,
    SwitchingProtocols = 101,
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,
    PartialContent = 206,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    RequestTimeout = 408,
    Conflict = 409,
    LengthRequired = 411,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    IMATeaPot = 418,
    UnprocessableEntity = 422,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    InternalServerError = 500,
    NotImplemented = 501,
    ServiceTemporarilyUnavailable = 503,
    GatewayTimeout = 504,
    InsufficientStorage = 507,
};

enum class APIVersion {
    Octoprint,
    v1
};

// TODO: Replace stringy URLs with tokens/enums.
// Note: The same buffer is also reused for the boundary. We can do that because:
// * We need the boundary only for short URLs.
// * The URL must come first, so we know how much space there's after that.
//
// That saves quite some space compared with having two buffers.
static const size_t MAX_URL_LEN = 168;
using Url = std::array<char, MAX_URL_LEN>;

// # of seconds after which nonce becomes stale for digest authentication
// The value of 300 has been chosen as it's the default value used in the Apache
// web server, see:
// https://httpd.apache.org/docs/2.4/mod/mod_auth_digest.html#authdigestnoncelifetime
//
// This value use to be much lower but would cause issues with Safari-based browser
// See https://github.com/prusa3d/Prusa-Firmware-Buddy/issues/3287
static const uint32_t valid_nonce_period = 300;

} // namespace http
