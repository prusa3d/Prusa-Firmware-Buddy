#include "connect.hpp"

#include "httpc.hpp"
#include <string.h>
#include <cmsis_os.h>
#include <log.h>

using namespace con;

LOG_COMPONENT_DEF(connect, LOG_SEVERITY_DEBUG);

void connect::run() {
    osDelay(10000);
    log_debug(connect, "connect thread starts");
    char url[] = { "ecdsa.ct.xln.cz" };
    uint16_t port_no = 443;

    while (1) {

        constexpr size_t client_buffer_len = 512;
        uint8_t client_buffer[client_buffer_len];

        class tls conn;

        http_client client { url, port_no, (uint8_t *)client_buffer, client_buffer_len, &conn };

        client.loop();

        osDelay(5000);
    }
}
