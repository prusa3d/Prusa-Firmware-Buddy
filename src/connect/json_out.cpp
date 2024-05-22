#include "json_out.hpp"
#include <http/types.h>
#include <segmented_json.h>

using http::ContentType;
using http::Error;
using http::Method;
using json::JsonResult;

namespace connect_client {

ContentType JsonPostRequest::content_type() const {
    return ContentType::ApplicationJson;
}

Method JsonPostRequest::method() const {
    return Method::Post;
}

JsonPostRequest::RenderResult JsonPostRequest::write_body_chunk(char *data, size_t size) {
    switch (progress) {
    case Progress::Done:
        return 0U;
    case Progress::Rendering: {
        const auto [result, written_json] = renderer().render(reinterpret_cast<uint8_t *>(data), size);
        switch (result) {
        case JsonResult::Abort:
            assert(0);
            progress = Progress::Done;
            return Error::InternalError;
        case JsonResult::BufferTooSmall:
            // Can't fit even our largest buffer :-(.
            //
            // (the http client flushes the headers before trying to
            // render the body, so we have a full buffer each time).
            progress = Progress::Done;
            return Error::InternalError;
        case JsonResult::Incomplete:
            return written_json;
        case JsonResult::Complete:
            progress = Progress::Done;
            return written_json;
        }
    }
    }
    assert(0);
    return Error::InternalError;
}

} // namespace connect_client
