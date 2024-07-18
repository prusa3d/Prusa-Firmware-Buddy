#include "status_renderer.h"

#include "marlin_client.hpp"
#include <marlin_vars.hpp>
#include <state/printer_state.hpp>
#include <transfers/monitor.hpp>
#include <segmented_json_macros.h>

#include <option/buddy_enable_connect.h>
#if BUDDY_ENABLE_CONNECT()
    #include <connect/connect.hpp>
#endif

using namespace marlin_server;
using transfers::Monitor;

namespace nhttp::handler {

json::JsonResult StatusRenderer::renderState(size_t resume_point, json::JsonOutput &output, StatusState &state) const {
    // Note: We allow stale, because if it was not there the state.transfer_id
    // will be nullopt and we will not send it anyway
    auto transfer_status = Monitor::instance.status(true);
    if (transfer_status.has_value() && transfer_status->id != state.transfer_id) {
        // if transfer changes mid report, bail out
        transfer_status.reset();
    }

    uint32_t time_to_end = marlin_vars().time_to_end;
    uint32_t time_to_pause = marlin_vars().time_to_pause;
    auto link_state = printer_state::get_state(false);

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
    if (printer_state::has_job()) {
        JSON_FIELD_OBJ("job");
            JSON_FIELD_INT("id", marlin_vars().job_id) JSON_COMMA;
            JSON_FIELD_FFIXED("progress", ((float)marlin_vars().sd_percent_done), 2) JSON_COMMA;
            if (time_to_end != TIME_TO_END_INVALID) {
                JSON_FIELD_INT("time_remaining", time_to_end) JSON_COMMA;
            }
            if (time_to_pause != TIME_TO_END_INVALID) {
                JSON_FIELD_INT("filament_change_in", time_to_pause) JSON_COMMA;
            }
            JSON_FIELD_INT("time_printing", marlin_vars().print_duration);
        JSON_OBJ_END JSON_COMMA;
    }
    JSON_FIELD_OBJ("storage");
        JSON_FIELD_STR("path", "/usb/") JSON_COMMA;
        JSON_FIELD_STR("name", "usb") JSON_COMMA;
        JSON_FIELD_BOOL("read_only", false);
    JSON_OBJ_END JSON_COMMA;
    if (state.transfer_id.has_value()) {
        JSON_FIELD_OBJ("transfer");
            JSON_FIELD_INT_G(transfer_status.has_value(), "id", transfer_status->id) JSON_COMMA;
            JSON_FIELD_INT_G(transfer_status.has_value(), "time_transferring", transfer_status->time_transferring()) JSON_COMMA;
            JSON_FIELD_FFIXED_G(transfer_status.has_value(), "progress", transfer_status->progress_estimate(), 2) JSON_COMMA;
            JSON_FIELD_INT_G(transfer_status.has_value(), "transferred", transfer_status->download_progress.get_valid_size());
        JSON_OBJ_END JSON_COMMA;
    }
        JSON_FIELD_OBJ("printer");
            JSON_FIELD_STR("state", printer_state::to_str(link_state)) JSON_COMMA;
            JSON_FIELD_FFIXED("temp_bed", marlin_vars().temp_bed, 1) JSON_COMMA;
            JSON_FIELD_FFIXED("target_bed", marlin_vars().target_bed, 1) JSON_COMMA;
            JSON_FIELD_FFIXED("temp_nozzle", marlin_vars().active_hotend().temp_nozzle, 1) JSON_COMMA;
            JSON_FIELD_FFIXED("target_nozzle", marlin_vars().active_hotend().target_nozzle, 1) JSON_COMMA;
            // XYZE, mm
            JSON_FIELD_FFIXED("axis_z", marlin_vars().logical_curr_pos[2], 1) JSON_COMMA;
            if (!marlin_client::is_printing()) {
                JSON_FIELD_FFIXED("axis_x", marlin_vars().logical_curr_pos[0], 1) JSON_COMMA;
                JSON_FIELD_FFIXED("axis_y", marlin_vars().logical_curr_pos[1], 1) JSON_COMMA;
            }
            JSON_FIELD_INT("flow", marlin_vars().active_hotend().flow_factor) JSON_COMMA;
            JSON_FIELD_INT("speed", marlin_vars().print_speed) JSON_COMMA;
            JSON_FIELD_INT("fan_hotend", marlin_vars().active_hotend().heatbreak_fan_rpm) JSON_COMMA;
            JSON_FIELD_INT("fan_print", marlin_vars().active_hotend().print_fan_rpm);
        JSON_OBJ_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

} // namespace nhttp::handler
