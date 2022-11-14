#pragma once

#include "buffer.hpp"
#include <http/httpc.hpp>
#include "planner.hpp"
#include "printer.hpp"

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
};

OnlineStatus last_status();

enum class RequestType {
    Telemetry,
    SendInfo,
};

class connect {
private:
    class CachedFactory;

    Planner planner;
    Printer &printer;
    SharedBuffer &buffer;
    std::optional<uint32_t> telemetry_fingerprint;
    // We want to make sure to send a full telemetry every now and then even if nothing changed.
    // (There seems to be a problem on the server, not being able to cope with that).
    uint32_t last_full_telemetry;

    using ServerResp = std::variant<std::monostate, Command, http::Error>;

    // transmission and reception with Connect server
    std::optional<OnlineStatus> communicate(CachedFactory &conn_factory);
    ServerResp handle_server_resp(http::Response response);

public:
    connect(Printer &printer, SharedBuffer &buffer);
    void run(void) __attribute__((noreturn));
};

}
