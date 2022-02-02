#include "httpc.hpp"
#include "connection.hpp"
#include "tls/tls.hpp"
#include <log.h>
#include <string.h>

namespace con {

LOG_COMPONENT_DEF(httpc, LOG_SEVERITY_DEBUG);

http_client::http_client(char *host, uint16_t port, uint8_t *const buffer, const size_t buffer_len, class Connection *con)
    : host { host }
    , port { port }
    , buffer { buffer }
    , buffer_len { buffer_len }
    , con { con } {
    valid = (Error::OK == con->connect(host, port)) ? true : false;
}

void http_client::loop() {
    if (!valid)
        return;

    constexpr const char send_data[] = { "POST /p/telemetry HTTP/1.1\r\nHost: ecdsa.ct.xln.cz\r\n"
                                         "Token: 1c7d91ee1dceda7a9f4e\r\nFingerprint: ODO6QEENODO6QEEN\r\n"
                                         "Content-Type: application/json\r\n"
                                         "Content-Length: 192\r\n\r\n"
                                         "{\"temp_nozzle\":24.22,\"temp_bed\":23.55,\"target_nozzle\":0.00,\"target_bed\":0.00,\"fan_print\":0,\"material\":\"---\",\"axis_x\":180.40,\"axis_y\":-3.00,\"axis_z\":0.00,\"speed\":100,\"flow\":100,\"state\":\"READY\"}" };
    size_t tx_data_len = strlen((const char *)send_data);
    strncpy((char *)buffer, send_data, buffer_len);
    tx_data_len = buffer_len < tx_data_len ? buffer_len : tx_data_len;
    std::variant<size_t, Error> ret = con->write(buffer, tx_data_len);
    if (!std::holds_alternative<size_t>(ret))
        return;
    memset(buffer, 0, buffer_len);
    ret = con->read(buffer, buffer_len);
    log_debug(httpc, "%s", buffer);
}

}
