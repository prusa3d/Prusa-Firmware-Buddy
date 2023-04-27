#include "status.hpp"

#include <cassert>

namespace connect_client {

bool status_replace_early(OnlineStatus status) {
    switch (status) {
    // Off states
    case OnlineStatus::Unknown:
    case OnlineStatus::Off:
    case OnlineStatus::NoConfig:
    case OnlineStatus::RegistrationRequesting:
    case OnlineStatus::RegistrationCode:
    case OnlineStatus::RegistrationDone:
    case OnlineStatus::RegistrationError:
    // Network-level/transient errors
    case OnlineStatus::NoDNS:
    case OnlineStatus::NoConnection:
    case OnlineStatus::Tls:
    case OnlineStatus::NetworkError:
        return true;
    // User errors
    case OnlineStatus::Auth:
    // "Bug"-level errors
    case OnlineStatus::ServerError:
    case OnlineStatus::InternalError:
    case OnlineStatus::Confused:
        // Common "happy" states.
    case OnlineStatus::Ok:
    case OnlineStatus::Connecting:
        return false;
    }
    assert(0); // Unreachable
    return false;
}

}
