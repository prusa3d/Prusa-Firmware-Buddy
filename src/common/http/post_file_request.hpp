#pragma once
#include "httpc.hpp"
#include "unique_file_ptr.hpp"

namespace http {
class PostFile : public Request {
public:
    PostFile(const char *file_path, const char *url_string, size_t file_size);
    virtual ~PostFile() = default;
    const char *url() const override;
    ContentType content_type() const override;
    Method method() const override;
    const HeaderOut *extra_headers() const override;
    std::variant<size_t, Error> write_body_chunk(char *data, size_t size) override;

private:
    HeaderOut hdrs[2];
    const char *url_string;
    unique_file_ptr file;
};
}; // namespace http
