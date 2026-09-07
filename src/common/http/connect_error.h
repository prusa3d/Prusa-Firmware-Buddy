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
    Memory,
    // Framing errors (in case of websocket)
    WebSocket,
    // Unexpected EOF,
    UnexpectedEOF,
    // „Unreacheable“ handling. Should not happen (and may be covered by some
    // debug_assert(0) somewhere and this being only a production filler).
    InternalError,
    // Proxy won't let us do what we want to.
    Proxy,
};

const char *to_str(Error error);

} // namespace http
