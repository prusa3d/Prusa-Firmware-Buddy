#pragma once

namespace con {

enum class Error {
    Timeout,
    Parse,
    UnexpectedResponse,
    SetSockOpt,
    ResponseTooLong,
    ERROR,
    CONNECTION_ERROR,
    WRITE_ERROR,
    READ_ERROR,
    INVALID_PARAMETER_ERROR,
    BUFFER_OVERFLOW_ERROR,
    MARLIN_CLIENT_ERROR,
    TLS_CERT_ERROR,
    // „Unreacheable“ handling. Should not happen (and may be covered by some
    // assert(0) somewhere and this being only a production filler).
    InternalError,
};

}
