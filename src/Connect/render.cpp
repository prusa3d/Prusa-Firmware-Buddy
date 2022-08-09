#include "render.hpp"

#include <segmented_json_macros.h>

#include <cassert>

using json::JsonOutput;
using json::JsonResult;
using std::visit;

namespace con {

namespace {

    const char *to_str(Printer::DeviceState state) {
        switch (state) {
        case Printer::DeviceState::Idle:
            return "IDLE";
        case Printer::DeviceState::Printing:
            return "PRINTING";
        case Printer::DeviceState::Paused:
            return "PAUSED";
        case Printer::DeviceState::Finished:
            return "FINISHED";
        case Printer::DeviceState::Prepared:
            return "PREPARED";
        case Printer::DeviceState::Error:
            return "ERROR";
        case Printer::DeviceState::Unknown:
        default:
            return "UNKNOWN";
        }
    }

    bool is_printing(Printer::DeviceState state) {
        return state == Printer::DeviceState::Printing || state == Printer::DeviceState::Paused;
    }

    JsonResult render_msg(size_t resume_point, JsonOutput &output, const RenderState &state, const SendTelemetry &telemetry) {
        const auto params = state.printer.params();
        const bool printing = is_printing(params.state);
        // Keep the indentation of the JSON in here!
        // clang-format off
        JSON_START;
        JSON_OBJ_START;
            if (!telemetry.empty) {
                JSON_FIELD_FFIXED("temp_nozzle", params.temp_nozzle, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("temp_bed", params.temp_bed, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("target_nozzle", params.target_nozzle, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("target_bed", params.target_bed, 1) JSON_COMMA;
                if (!printing) {
                    // To avoid spamming the DB, connect doesn't want positions during printing
                    JSON_FIELD_FFIXED("axis_x", params.pos[Printer::X_AXIS_POS], 2) JSON_COMMA;
                    JSON_FIELD_FFIXED("axis_y", params.pos[Printer::Y_AXIS_POS], 2) JSON_COMMA;
                }
                JSON_FIELD_FFIXED("axis_z", params.pos[Printer::Z_AXIS_POS], 2) JSON_COMMA;
                if (printing) {
                    JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
                    JSON_FIELD_INT("speed", params.print_speed) JSON_COMMA;
                    JSON_FIELD_INT("flow", params.flow_factor) JSON_COMMA;
                    JSON_FIELD_INT("time_printing", params.print_duration) JSON_COMMA;
                    JSON_FIELD_INT("time_remaining", params.time_to_end) JSON_COMMA;
                    JSON_FIELD_INT("progress", params.progress_percent) JSON_COMMA;
                    JSON_FIELD_INT("fan_extruder", params.heatbreak_fan_rpm) JSON_COMMA;
                    JSON_FIELD_INT("fan_print", params.print_fan_rpm) JSON_COMMA;
                    JSON_FIELD_FFIXED("filament", params.filament_used, 1) JSON_COMMA;
                }
                JSON_FIELD_STR("state", to_str(params.state));
            }
        JSON_OBJ_END;
        JSON_END;
        // clang-format on
    }

    JsonResult render_msg(size_t resume_point, JsonOutput &output, const RenderState &state, const Event &event) {
        const auto params = state.printer.params();
        const auto &info = state.printer.printer_info();
        const bool has_extra = (event.type != EventType::Accepted) && (event.type != EventType::Rejected);
        const bool printing = is_printing(params.state);

        if (event.type == EventType::JobInfo && (!printing || event.job_id != params.job_id)) {
            // Can't send a job info when not printing, refuse instead.
            //
            // Can't provide historic/future jobs.
            //
            // (The fact we can render in multiple steps doesn't matter, we would
            // descend into here every time and resume the Rejected event).
            Event rejected(event);
            rejected.type = EventType::Rejected;
            return render_msg(resume_point, output, state, rejected);
        }

        // Keep the indentation of the JSON in here!
        // clang-format off
        JSON_START;
        JSON_OBJ_START;
            if (has_extra && printing) {
                JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
            }

            // Relevant "data" block, if any

            // Note: this would very much like to be a switch. Nevertheless, the
            // JSON_START/macros are already a big and quite nasty switch, and the
            // JSON_... macros don't work in a nested switch :-(.
            if (event.type == EventType::Info) {
                JSON_FIELD_OBJ("data");
                    JSON_FIELD_STR("firmware", info.firmware_version) JSON_COMMA;
                    JSON_FIELD_STR("sn", info.serial_number) JSON_COMMA;
                    JSON_FIELD_BOOL("appendix", info.appendix) JSON_COMMA;
                    JSON_FIELD_STR("fingerprint", info.fingerprint);
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::JobInfo) {
                JSON_FIELD_OBJ("data");
                    JSON_FIELD_STR("path", params.job_path);
                JSON_OBJ_END JSON_COMMA;
            }

            JSON_FIELD_INT("command_id", event.command_id.value_or(0)) JSON_COMMA;
            JSON_FIELD_STR("event", to_str(event.type));
        JSON_OBJ_END;
        JSON_END;
        // clang-format on
    }

    JsonResult render_msg(size_t, JsonOutput &, const RenderState &, const Sleep &) {
        // Sleep is handled on upper layers, not through renderer.
        assert(0);
        return JsonResult::Abort;
    }

}

JsonResult Renderer::renderState(size_t resume_point, JsonOutput &output, RenderState &state) const {
    return visit([&](auto action) -> JsonResult {
        return render_msg(resume_point, output, state, action);
    },
        state.action);
}

}
