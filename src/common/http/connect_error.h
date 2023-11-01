#pragma once

namespace http {

enum class Error {
    Timeout,
    Parse,
    SetSockOpt,
    ResponseTooLong,
    Dns,
    Connect,
    Network,
    Tls,
    // „Unreacheable“ handling. Should not happen (and may be covered by some
    // assert(0) somewhere and this being only a production filler).
    InternalError,
};

const char *to_str(Error error);

} // namespace http
