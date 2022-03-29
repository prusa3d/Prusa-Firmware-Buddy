#pragma once

#include <stdint.h>
#include "httpc.hpp"
#include "core_interface.hpp"

namespace con {

class connect {

private:
    REQUEST_TYPE req_type = REQUEST_TYPE::SEND_INFO; // next HTTP request to send to Connect server
    core_interface core;                             // interface to core functionalities (marlin, network, etc.)

    // gets the data to send to Connect server
    std::variant<size_t, Error> get_data_to_send(uint8_t *buffer, size_t buffer_len, printer_info_t *printer_info);

    // transmission and reception with Connect server
    void communicate();

public:
    void run(void);
};

}
