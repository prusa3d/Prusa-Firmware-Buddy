#pragma once

#include "httpc.hpp"

namespace http {

class GetRequest : public Request {
private:
    const char *url_path;
    const HeaderOut *hdrs;

public:
    virtual const char *url() const override {
        return url_path;
    }
    virtual Method method() const override {
        return Method::Get;
    }
    virtual ContentType content_type() const override {
        // This doesn't matter. As the GET method is body-less, this function
        // is not called. But we need to provide anyway to make C++ happy.
        return ContentType::ApplicationOctetStream;
    }
    virtual const HeaderOut *extra_headers() const override {
        return hdrs;
    }
    GetRequest(const char *url_path, const HeaderOut *hdrs = nullptr)
        : url_path(url_path)
        , hdrs(hdrs) {}
};

} // namespace http
