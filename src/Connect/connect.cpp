#include "connect.hpp"
#include "httpc.hpp"
#include "httpc_data.hpp"

#include <debug.h>
#include <os_porting.hpp>
#include <cstring>
#include <optional>
#include <socket.hpp>
#include <cmsis_os.h>

#include <log.h>

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
    // TODO: Choose if we want TLS or not.
#if 0
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
#endif

    constexpr size_t client_buffer_len = 512;
    uint8_t client_buffer[client_buffer_len];
    std::variant<size_t, Error> ret;
    printer_info_t printer_info;
    configuration_t config;

    std::optional<Error> err = core.get_printer_info(&printer_info);
    if (err.has_value())
        return;
    core.get_connect_config(&config);
    if (err.has_value())
        return;

    class socket_con conn;
    http_client client { config.url, config.port, (class Connection *)&conn };
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
    configuration_t config;
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

    // In the ideal world this step has to be done frequently to update the
    // changes. Once there is some mechanism to get update from FW. This must be done
    // differently
    while (true) {
        std::optional<Error> ret = core.get_connect_config(&config);
        if (!ret.has_value())
            break;
        osDelay(10000);
    }
    //log_debug(connect, "Valid connect configuration found\n");
    CONNECT_DEBUG("url: %s\n", config.url);
    CONNECT_DEBUG("Port: %u\n", config.port);
    CONNECT_DEBUG("Token: %s\n", config.token);

    while (true) {
        communicate();
        // Connect server expects telemetry at least every 30 s (varies with design decisions).
        // So the client has to communicate very frequently with the server!
        osDelay(1000);
    }
}

}
