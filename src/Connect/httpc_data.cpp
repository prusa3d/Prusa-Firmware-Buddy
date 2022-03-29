#include "httpc_data.hpp"
#include "core_interface.hpp"
#include "os_porting.hpp"
#include <cstring>
#include <cstdio>
#include <cinttypes>

namespace con {

std::variant<size_t, Error> httpc_data::telemetry(device_params_t *params, char *buffer, size_t buffer_len) {
    constexpr size_t len = 10;
    char device_state_string[len];

    switch (params->state) {
    case DEVICE_STATE_UNKNOWN:
        strlcpy(device_state_string, "UNKNOWN", len);
        break;
    case DEVICE_STATE_READY:
        strlcpy(device_state_string, "READY", len);
        break;
    case DEVICE_STATE_PRINTING:
        strlcpy(device_state_string, "PRINTING", len);
        break;
    case DEVICE_STATE_PAUSED:
        strlcpy(device_state_string, "PAUSED", len);
        break;
    case DEVICE_STATE_FINISHED:
        strlcpy(device_state_string, "FINISHED", len);
        break;
    case DEVICE_STATE_PREPARED:
        strlcpy(device_state_string, "PREPARED", len);
        break;
    case DEVICE_STATE_ERROR:
        strlcpy(device_state_string, "ERROR", len);
        break;
    default:
        return Error::INVALID_PARAMETER_ERROR;
        break;
    }

    int bytes_written = snprintf(
        buffer, buffer_len,
        "{"
        "\"temp_nozzle\":%.2f,"
        "\"temp_bed\":%.2f,"
        "\"target_nozzle\":%.2f,"
        "\"target_bed\":%.2f,"
        "\"axis_x\":%.2f,"
        "\"axis_y\":%.2f,"
        "\"axis_z\":%.2f,"
        "\"speed\":%" PRIu16 ","
        "\"flow\":%" PRIu16 ","
        "\"state\":\"%s\""
        "}",
        (double)params->temp_nozzle,
        (double)params->temp_bed,
        (double)params->target_nozzle,
        (double)params->target_bed,
        (double)params->pos[X_AXIS_POS],
        (double)params->pos[Y_AXIS_POS],
        (double)params->pos[Z_AXIS_POS],
        params->print_speed,
        params->flow_factor,
        device_state_string);

    std::variant<size_t, Error> ret;

    if ((bytes_written >= (int)buffer_len) || (bytes_written < 0))
        ret = Error::ERROR;
    else
        ret = (size_t)bytes_written;

    return ret;
}

std::variant<size_t, Error> httpc_data::info(printer_info_t *info, char *buffer, const uint32_t buffer_len, uint32_t command_id) {
    int bytes_written = snprintf(
        buffer, buffer_len,
        "{"
        "\"event\":\"INFO\","
        "\"command_id\":%" PRIu32 ","
        "\"data\": {" // data object start
        "\"firmware\":\"%s\","
        "\"sn\":\"%s\","
        "\"appendix\":%s,"
        "\"fingerprint\":\"%s\","
        "\"type\":%hhu"
        "}" // data end
        "}",
        command_id, info->firmware_version, info->serial_number,
        info->appendix ? "true" : "false", info->fingerprint, info->printer_type);

    std::variant<size_t, Error> ret;
    if ((bytes_written >= (int)buffer_len) || (bytes_written < 0))
        ret = Error::ERROR;
    else
        ret = (size_t)bytes_written;

    return ret;
}

}
