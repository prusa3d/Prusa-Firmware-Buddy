#pragma once
#include "httpc.hpp"
#include <functional>

namespace http {
/**
* @brief Post Request with 'write_body_chunk' given by user via a (potentially stateful) lambda
*
*/
class PostLambda : public Request {
public:
    using LambdaT = std::function<std::variant<size_t, Error>(char *, size_t)>;

    PostLambda(LambdaT lambda, const char *url_string);
    virtual ~PostLambda() = default;
    const char *url() const override;
    ContentType content_type() const override;
    Method method() const override;
    std::variant<size_t, Error> write_body_chunk(char *data, size_t size) override;

private:
    const char *url_string;
    LambdaT lamb;
};
};
