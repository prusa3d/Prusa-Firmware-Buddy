#pragma once

#include <array>
#include <cstdlib>

namespace nhttp {

enum Method {
    Head,
    Get,
    Post,
    Put,
    Delete,
    UnknownMethod,
};

enum class ContentType {
    TextPlain,
    TextHtml,
    TextCss,
    ImageIco,
    ImagePng,
    ImageSvg,
    ApplicationJavascript,
    ApplicationJson,
    ApplicationOctetStream,
};

enum class ConnectionHandling {
    Close,
    ContentLengthKeep,
    ChunkedKeep,
};

// Note: Not exhaustive! Therefore not enum class, just enum.
enum Status {
    UnknownStatus = 0,
    Ok = 200,
    Created = 201,
    NoContent = 204,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    Conflict = 409,
    LengthRequired = 411,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    IMATeaPot = 418,
    UnprocessableEntity = 422,
    RequestHeaderFieldsTooLarge = 431,
    InternalServerError = 500,
    NotImplemented = 501,
    ServiceTemporarilyUnavailable = 503,
    InsufficientStorage = 507,
};

// TODO: Replace stringy URLs with tokens/enums.
// Note: The same buffer is also reused for the boundary. We can do that because:
// * We need the boundary only for short URLs.
// * The URL must come first, so we know how much space there's after that.
//
// That saves quite some space compared with having two buffers.
static const size_t MAX_URL_LEN = 100;
using Url = std::array<char, MAX_URL_LEN>;

class Server;

namespace handler {
    struct Step;
}

}
