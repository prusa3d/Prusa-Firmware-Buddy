#include "post_lambda_request.hpp"

namespace http {
PostLambda::PostLambda(LambdaT lam, const char *url_string)
    : url_string(url_string)
    , lamb(lam) {
}
const char *PostLambda::url() const {
    return url_string;
}
ContentType PostLambda::content_type() const {
    return ContentType::ApplicationOctetStream;
}
Method PostLambda::method() const {
    return Method::Post;
}

std::variant<size_t, Error> PostLambda::write_body_chunk(char *data, size_t size) {
    assert(size && lamb);
    if (!size || !lamb) {
        return Error::InternalError;
    }

    return lamb(data, size);
}

};
