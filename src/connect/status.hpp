#pragma once

#include <common/http/connect_error.h>

#include <tuple>
#include <variant>
#include <cstdint>
#include <optional>

namespace connect_client {

enum class OnlineError {
    NoError,
    Dns,
    Connection,
    Tls,
    Auth,
    Server,
    Internal,
    Network,
    Confused,
};

enum class ConnectionStatus {
    Unknown,
    Off,
    NoConfig,
    Ok,
    Connecting, ///< Connecting in progress but no result yet
    Error,
    RegistrationRequesting,
    RegistrationCode,
    RegistrationDone,
    RegistrationError,
};

struct ErrWithRetry {
    OnlineError err;
    // nullopt -> no limit, retry infinitely
    // On 0 -> also transition to the error state as provided.
    std::optional<uint8_t> retry;
};

// Current status, last error (that may be left from previous attempt) and number of retries before this error becomes a problem and "something bad happens"
using OnlineStatus = std::tuple<ConnectionStatus, OnlineError, std::optional<uint8_t>>;

using CommResult = std::variant<std::monostate, ConnectionStatus, OnlineError, ErrWithRetry>;

OnlineError err_to_status(http::Error error);

} // namespace connect_client
