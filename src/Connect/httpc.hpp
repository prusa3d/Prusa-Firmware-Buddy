#pragma once

#include "tls/tls.hpp"

#include "connection.hpp"

#include <stdint.h>

namespace con {

class http_client {

private:
    bool valid = true;
    char *host;
    uint16_t port;
    uint8_t *buffer;
    size_t buffer_len;
    Connection *con;

public:
    http_client(char *host, uint16_t port, uint8_t *const buffer, const size_t buffer_len, class Connection *con);

    void loop();
};
}
