#include "connect.hpp"
#include "httpc.hpp"
#include "httpc_data.hpp"

#include <debug.h>
#include <os_porting.hpp>
#include <cstring>
#include <optional>
#include <variant>
#include <socket.hpp>
#include <cmsis_os.h>

#include <log.h>

using std::variant;

LOG_COMPONENT_DEF(connect, LOG_SEVERITY_DEBUG);

namespace con {

std::variant<size_t, Error> connect::get_data_to_send(uint8_t *buffer, size_t buffer_len, printer_info_t *printer_info) {
    std::variant<size_t, Error> ret;
    httpc_data data;

    switch (req_type) {
    case REQUEST_TYPE::SEND_INFO:
        ret = data.info(printer_info, (char *)buffer, buffer_len, 0);
        break;
    case REQUEST_TYPE::TELEMETRY:
        device_params_t params;
        core.get_data(&params);
        ret = data.telemetry(&params, (char *)buffer, buffer_len);
        break;
    default:
        ret = Error::INVALID_PARAMETER_ERROR;
        break;
    }

    return ret;
}

void connect::communicate() {
    configuration_t config = core.get_connect_config();

    if (!config.enabled) {
        return;
    }

    std::variant<size_t, Error> ret;

    printer_info_t printer_info;
    std::optional<Error> err = core.get_printer_info(&printer_info);
    if (err.has_value())
        return;

    constexpr size_t client_buffer_len = 512;
    uint8_t client_buffer[client_buffer_len];

    // TODO: Any nicer way to do this in C++?
    variant<tls, socket_con> connection_storage;
    Connection *connection;
    if (config.tls) {
        connection_storage.emplace<tls>();
        connection = &std::get<tls>(connection_storage);
    } else {
        connection_storage.emplace<socket_con>();
        connection = &std::get<socket_con>(connection_storage);
    }

    http_client client { config.host, config.port, connection };
    if (!client.is_connected())
        return;

    ret = get_data_to_send(client_buffer, client_buffer_len, &printer_info);
    if (!std::holds_alternative<size_t>(ret))
        return;

    size_t body_length = std::get<size_t>(ret);

    err = client.send_header(req_type, printer_info.fingerprint, config.token, body_length);
    if (err.has_value())
        return;

    err = client.send_body(client_buffer, client_buffer_len);
    if (err.has_value())
        return;

    // INFO is send only once, and TELEMETRY afterwards.
    req_type = REQUEST_TYPE::TELEMETRY;
}

void connect::run() {
    CONNECT_DEBUG("%s", "Connect client starts\n");
    // waits for file-system and network interface to be ready
    //FIXME! some mechanisms to know that file-system and network are ready.
    osDelay(10000);

    printer_info_t printer_info;
    std::optional<Error> ret = core.get_printer_info(&printer_info);
    // without valid printer info connect won't work!
    if (ret.has_value()) {
        // FIXME: Make work with namespaces
        //log_debug(connect, "Ending Connect Thread due to invalid printer info!");
        osThreadId id = osThreadGetId();
        osThreadTerminate(id);
    }
    //log_debug(connect, "Valid printer parameters found\n");
    CONNECT_DEBUG("Firmware: %s\n", printer_info.firmware_version);
    CONNECT_DEBUG("Printer Type: %hhu\n", printer_info.printer_type);
    CONNECT_DEBUG("Fingerprint: %s\n", printer_info.fingerprint);
    //log_debug(connect, "Valid connect configuration found\n");

    while (true) {
        communicate();
        // Connect server expects telemetry at least every 30 s (varies with design decisions).
        // So the client has to communicate very frequently with the server!
        osDelay(10000);
    }
}

}
