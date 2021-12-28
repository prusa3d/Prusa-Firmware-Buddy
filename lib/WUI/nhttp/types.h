#pragma once

#include <array>
#include <cstdlib>

namespace nhttp {

enum class Method {
    Head,
    Get,
    Post,
    Put,
    Delete,
    Unknown,
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
enum Status : uint16_t {
    Unknown = 0,
    Ok = 200,
    NoContent = 204,
    BadRequest = 400,
    Unauthorized = 401,
    NotFound = 404,
    MethodNotAllowerd = 405,
    Conflict = 409,
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
static const size_t MAX_URL_LEN = 64;
using Url = std::array<char, MAX_URL_LEN>;

class Server;

namespace handler {
    struct Step;
}

}
