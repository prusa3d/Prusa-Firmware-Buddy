#pragma once

#include "changes.hpp"
#include "planner.hpp"
#include "printer.hpp"

#include <http/httpc.hpp>
#include <common/shared_buffer.hpp>

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
};

OnlineStatus last_status();

enum class RequestType {
    Telemetry,
    SendInfo,
};

class Connect {
private:
    class CachedFactory;

    Tracked telemetry_changes;
    // We want to make sure to send a full telemetry every now and then even if nothing changed.
    // (There seems to be a problem on the server, not being able to cope with that).
    //
    // This is in addition to the telemetry changes tracker.
    uint32_t last_full_telemetry;
    Planner planner;
    Printer &printer;
    SharedBuffer &buffer;

    using ServerResp = std::variant<std::monostate, Command, http::Error>;

    // transmission and reception with Connect server
    std::optional<OnlineStatus> communicate(CachedFactory &conn_factory);
    ServerResp handle_server_resp(http::Response response, CommandId command_id);
    Connect(const Connect &other) = delete;
    Connect(Connect &&other) = delete;

public:
    Connect(Printer &printer, SharedBuffer &buffer);
    void run(void) __attribute__((noreturn));
};

}
