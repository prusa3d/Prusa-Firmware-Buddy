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

/// Should the status be replaced by "Connecting" as early as possible?
///
/// With some states, we want to keep them shown to the user (mostly, user
/// errors). For some, we prefer to indicate "Connecting" to show that
/// something is indeed happening. These are the states where the user probably
/// did some changes and likes a feedback.
bool status_replace_early(OnlineStatus status);

}
