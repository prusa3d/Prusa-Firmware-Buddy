#pragma once

#include <stdint.h>
#include "httpc.hpp"
#include "core_interface.hpp"

namespace con {

enum class RequestType {
    Telemetry,
    SendInfo,
};

class connect {

private:
    RequestType next_request = RequestType::SendInfo;
    core_interface core; // interface to core functionalities (marlin, network, etc.)
    printer_info_t printer_info;

    // transmission and reception with Connect server
    void communicate();

public:
    connect();
    void run(void);
};

}
