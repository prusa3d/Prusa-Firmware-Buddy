#pragma once

namespace con {

enum class Error {
    ERROR,
    CONNECTION_ERROR,
    WRITE_ERROR,
    READ_ERROR,
    INVALID_PARAMETER_ERROR,
    BUFFER_OVERFLOW_ERROR,
    MARLIN_CLIENT_ERROR,
    TLS_CERT_ERROR,
};

}
