#include <connect_error.h>

namespace http {

const char *to_str(Error error) {
    switch (error) {
    case Error::Timeout:
        return "Timetout";
    case Error::Parse:
        return "Parse";
    case Error::SetSockOpt:
        return "Set sock opts";
    case Error::ResponseTooLong:
        return "Response too long";
    case Error::Dns:
        return "DNS";
    case Error::Connect:
        return "Connection";
    case Error::Network:
        return "Network";
    case Error::Memory:
        return "Memory";
    case Error::Tls:
        return "TLS";
    case Error::UnexpectedEOF:
        return "EOF";
    case Error::WebSocket:
        return "Websocket";
    case Error::InternalError:
        return "Internal error";
    }

    // To avoid the "reaches end" warning
    return "---";
}

} // namespace http
