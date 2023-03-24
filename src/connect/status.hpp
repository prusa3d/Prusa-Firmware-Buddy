#pragma once

namespace connect_client {

enum class OnlineStatus {
    Unknown,
    Off,
    NoConfig,
    NoDNS,
    NoConnection,
    Tls,
    Auth,
    ServerError,
    InternalError,
    NetworkError,
    Confused,
    Ok,
    Connecting, ///< Connecting in progress but no result yet
    RegistrationRequesting,
    RegistrationCode,
    RegistrationDone,
    RegistrationError,
};

}
