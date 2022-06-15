#include "types.h"

#include <cassert>

namespace http {

const char *to_str(ContentType content_type) {
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

}
