#pragma once

#include <stdint.h>
#include "httpc.hpp"
#include "planner.hpp"
#include "core_interface.hpp"

namespace con {

enum class RequestType {
    Telemetry,
    SendInfo,
};

class connect {

private:
    class CachedFactory;

    Planner planner;
    core_interface core; // interface to core functionalities (marlin, network, etc.)
    printer_info_t printer_info;

    using ServerResp = std::variant<std::monostate, Command, Error>;

    // transmission and reception with Connect server
    std::optional<Error> communicate(CachedFactory &conn_factory);
    ServerResp handle_server_resp(Response response);

public:
    connect();
    void run(void);
};

}
