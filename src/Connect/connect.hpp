#pragma once

#include "httpc.hpp"
#include "planner.hpp"
#include "printer.hpp"

namespace con {

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

    using ServerResp = std::variant<std::monostate, Command, Error>;

    // transmission and reception with Connect server
    std::optional<OnlineStatus> communicate(CachedFactory &conn_factory);
    ServerResp handle_server_resp(Response response);

public:
    connect(Printer &printer);
    void run(void) __attribute__((noreturn));
};

}
