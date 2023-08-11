#include "post_file_request.hpp"

namespace http {
PostFile::PostFile(const char *file_path, const char *url_string_, size_t file_size)
    : hdrs {
        { "Content-Length", file_size, std::nullopt },
        { nullptr, nullptr, std::nullopt },
    }
    , url_string(url_string_)
    , file(fopen(file_path, "rb")) {
}
const char *PostFile::url() const {
    return url_string;
}
ContentType PostFile::content_type() const {
    return ContentType::ApplicationOctetStream;
}
Method PostFile::method() const {
    return Method::Post;
}
const HeaderOut *PostFile::extra_headers() const {
    return hdrs;
}
std::variant<size_t, Error> PostFile::write_body_chunk(char *data, size_t size) {
    if (!file || !size) {
        return Error::InternalError;
    }

    if (auto written = fread(data, 1, size, file.get()); ferror(file.get())) {
        return Error::InternalError;
    } else {
        return written;
    }
}

}; // namespace http
