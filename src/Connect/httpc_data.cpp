#include "httpc_data.hpp"
#include "core_interface.hpp"
#include "os_porting.hpp"
#include <json_encode.h>
#include <cstring>
#include <cstdio>
#include <cinttypes>

namespace con {

namespace {

    const char *to_str(const device_state &state) {
        switch (state) {
        case DEVICE_STATE_READY:
            return "IDLE";
        case DEVICE_STATE_PRINTING:
            return "PRINTING";
        case DEVICE_STATE_PAUSED:
            return "PAUSED";
        case DEVICE_STATE_FINISHED:
            return "FINISHED";
        case DEVICE_STATE_PREPARED:
            return "PREPARED";
        case DEVICE_STATE_ERROR:
            return "ERROR";
        case DEVICE_STATE_UNKNOWN:
        default:
            return "UNKNOWN";
        }
    }

}

std::variant<size_t, Error> httpc_data::telemetry(const device_params_t &params, char *buffer, size_t buffer_len) {
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
        "\"job_id\":%" PRIu16 ","
        "\"state\":\"%s\""
        "}",
        (double)params.temp_nozzle,
        (double)params.temp_bed,
        (double)params.target_nozzle,
        (double)params.target_bed,
        (double)params.pos[X_AXIS_POS],
        (double)params.pos[Y_AXIS_POS],
        (double)params.pos[Z_AXIS_POS],
        params.print_speed,
        params.flow_factor,
        params.job_id,
        to_str(params.state));

    std::variant<size_t, Error> ret;

    if ((bytes_written >= (int)buffer_len) || (bytes_written < 0))
        ret = Error::ERROR;
    else
        ret = (size_t)bytes_written;

    return ret;
}

std::variant<size_t, Error> httpc_data::info(const printer_info_t &info, char *buffer, const uint32_t buffer_len, uint32_t command_id) {
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
        command_id, info.firmware_version, info.serial_number,
        info.appendix ? "true" : "false", info.fingerprint, info.printer_type);

    std::variant<size_t, Error> ret;
    if ((bytes_written >= (int)buffer_len) || (bytes_written < 0))
        ret = Error::ERROR;
    else
        ret = (size_t)bytes_written;

    return ret;
}

std::variant<size_t, Error> httpc_data::job_info(const device_params_t &params, char *buffer, const uint32_t buffer_len, uint32_t command_id) {
    // TODO: Is it OK that we send this even if we are no longer printing and it can be somewhat "stale"?
    const char *path = params.job_path;
    JSONIFY_STR(path);
    int bytes_written = snprintf(
        buffer, buffer_len,
        "{"
        "\"event\":\"JOB_INFO\","
        "\"command_id\":%" PRIu32 ","
        "\"job_id\":%" PRIu16 ","
        "\"data\": {" // data object start
        "\"path\": \"%s\""
        "}," // data end
        "\"state\": \"%s\""
        "}",
        command_id,
        params.job_id,
        path_escaped,
        to_str(params.state));

    std::variant<size_t, Error> ret;
    if ((bytes_written >= (int)buffer_len) || (bytes_written < 0))
        ret = Error::ERROR;
    else
        ret = (size_t)bytes_written;

    return ret;
}

}
