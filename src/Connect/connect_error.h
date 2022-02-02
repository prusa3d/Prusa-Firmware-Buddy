#pragma once

namespace con {

enum class Error {
    OK = 0,
    ERROR,
    CONNECTION_ERROR,
    TLS_CERT_ERROR,
    WRITE_ERROR,
    READ_ERROR,
};

}
