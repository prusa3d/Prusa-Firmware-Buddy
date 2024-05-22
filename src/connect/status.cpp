#include "status.hpp"

using http::Error;

namespace connect_client {

OnlineError err_to_status(Error error) {
    switch (error) {
    case Error::Connect:
        return OnlineError::Connection;
    case Error::Dns:
        return OnlineError::Dns;
    case Error::InternalError:
    case Error::ResponseTooLong:
    case Error::SetSockOpt:
    case Error::Memory:
        return OnlineError::Internal;
    case Error::Network:
    case Error::Timeout:
    case Error::UnexpectedEOF:
        return OnlineError::Network;
    case Error::Parse:
        return OnlineError::Confused;
    case Error::Tls:
        return OnlineError::Tls;
    default:
        return OnlineError::NoError;
    }
}

} // namespace connect_client
