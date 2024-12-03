#include "basic_gets.h"
#include "filament.hpp"
#include "marlin_client.hpp"
#include "lwip/init.h"
#include "netdev.h"
#include <config_store/store_instance.hpp>

#include <segmented_json_macros.h>
#include <json_encode.h>
#include <config_features.h>
#include <otp.hpp>
#include <filepath_operation.h>
#include <filename_type.hpp>
#include <state/printer_state.hpp>

#include <dirent.h>
#include <cstring>
#include <cstdio>
#include "printers.h"
#include <option/has_mmu2.h>

using namespace json;
using namespace marlin_server;

using printer_state::DeviceState;

namespace nhttp::link_content {

JsonResult get_printer(size_t resume_point, JsonOutput &output) {
    // Note about the marlin vars: It's true that on resumption we may get
    // different values. But they would still be reasonably "sane". If we eg.
    // finish a print and base what we include on previous version, we may
    // outdated values, but they are still there.
    marlin_vars_t &vars = marlin_vars();
    const FilamentType filament = config_store().get_filament_type(vars.active_extruder);
    const FilamentTypeParameters filament_material = filament.parameters();

    bool operational = true;
    bool paused = false;
    bool printing = false;
    bool cancelling = false;
    bool pausing = false;
    bool ready = true;
    bool busy = false;
    bool error = false;
    const char *link_state_str = nullptr;

    switch (vars.print_state) {
    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_XY_Measure:
#if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup:
#endif
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
    case State::PowerPanic_acFault:
    case State::PrintPreviewConfirmed:
    case State::SerialPrintInit:
        link_state_str = "BUSY";
        busy = true;
        [[fallthrough]];
    case State::Printing:
        printing = true;
        ready = operational = false;
        link_state_str = "PRINTING";
        break;
    case State::PowerPanic_AwaitingResume:
    case State::Pausing_Begin:
    case State::Pausing_Failed_Code:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
        printing = pausing = paused = busy = true;
        ready = operational = false;
        link_state_str = "PAUSED";
        break;
    case State::Paused:
        printing = paused = true;
        ready = operational = false;
        link_state_str = "PAUSED";
        break;
    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE:
    case State::PowerPanic_Resume:
        ready = operational = false;
        busy = printing = true;
        link_state_str = "PRINTING";
        break;
    case State::Aborting_Begin:
    case State::Aborting_WaitIdle:
    case State::Aborting_UnloadFilament:
    case State::Aborting_ParkHead:
    case State::Aborting_Preview:
        cancelling = busy = true;
        ready = operational = false;
        link_state_str = "BUSY";
        break;
    case State::Finishing_WaitIdle:
    case State::Finishing_UnloadFilament:
    case State::Finishing_ParkHead:
        busy = true;
        ready = operational = false;
        link_state_str = "BUSY";
        break;
    case State::Aborted:
    case State::Finished:
    case State::Exit:
    case State::Idle:
    case State::WaitGui:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
#endif
    case State::PrintInit:
        break;
    case State::PrintPreviewQuestions:
        // The "preview" is abused to ask questions about the filament and such.
        busy = printing = error = true;
        link_state_str = "ATTENTION";
        break;
    }

    // This is a bit of a hacky solution, but using the get_state and still getting all the bools correct and not
    // accidentally messing something up seems too much work given we want to hopefully soon enough implement the new v1 api
    // and stop using this. This way we get only the new attention states without any risky changes.
    auto link_state = printer_state::get_state(false);
    if (link_state == DeviceState::Attention) {
        link_state_str = "ATTENTION";
        busy = printing = error = true;
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_OBJ("telemetry");
            JSON_FIELD_FFIXED("temp-bed", vars.temp_bed, 1) JSON_COMMA;
            JSON_FIELD_FFIXED("temp-nozzle", vars.active_hotend().temp_nozzle, 1) JSON_COMMA;
            JSON_FIELD_INT("print-speed", vars.print_speed) JSON_COMMA;
            // XYZE, mm
            JSON_FIELD_FFIXED("z-height", vars.logical_pos[2], 1) JSON_COMMA;
            JSON_FIELD_STR("material", filament_material.name.data());
        JSON_OBJ_END JSON_COMMA;

        JSON_FIELD_OBJ("temperature");
            JSON_FIELD_OBJ("tool0");
                JSON_FIELD_FFIXED("actual", vars.hotend(0).temp_nozzle, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("target", vars.hotend(0).target_nozzle, 1) JSON_COMMA;
                // Note: our own extension, because our printers sometimes display
                // different "target" temperature than what they heat towards.
                JSON_FIELD_FFIXED("display", vars.hotend(0).display_nozzle, 1) JSON_COMMA;
                JSON_FIELD_INT("offset", 0);
            JSON_OBJ_END JSON_COMMA;
            JSON_FIELD_OBJ("bed");
                JSON_FIELD_FFIXED("actual", vars.temp_bed, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("target", vars.target_bed, 1) JSON_COMMA;
                JSON_FIELD_INT("offset", 0);
            JSON_OBJ_END;
        JSON_OBJ_END JSON_COMMA;

        JSON_FIELD_OBJ("state")
            JSON_FIELD_STR("text", printing ? "Printing" : "Operational") JSON_COMMA;
            JSON_FIELD_OBJ("flags");
                if (link_state_str != nullptr) {
                    JSON_FIELD_STR("link_state", link_state_str) JSON_COMMA;
                }
                JSON_FIELD_BOOL("operational", operational) JSON_COMMA;
                JSON_FIELD_BOOL("paused", paused) JSON_COMMA;
                JSON_FIELD_BOOL("printing", printing) JSON_COMMA;
                JSON_FIELD_BOOL("cancelling", cancelling) JSON_COMMA;
                JSON_FIELD_BOOL("pausing", pausing) JSON_COMMA;
                JSON_FIELD_BOOL("error", error) JSON_COMMA;
                // We don't have an SD card!
                JSON_CONTROL("\"sdReady\":false,\"closedOnError\":false,");
                JSON_FIELD_BOOL("ready", ready) JSON_COMMA;
                JSON_FIELD_BOOL("busy", busy);
            JSON_OBJ_END;
        JSON_OBJ_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format off
}

JsonResult get_version(size_t resume_point, JsonOutput &output) {
    char hostname[HOSTNAME_LEN + 1];
    netdev_get_hostname(netdev_get_active_id(), hostname, sizeof hostname);
    float nozzle_diameter = config_store().get_nozzle_diameter(0);

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_STR("api", PL_VERSION_STRING) JSON_COMMA;
        // FIXME: The server version is probably bogus. But it's unclear what
        // the version is supposed to mean anyway. Waiting for the new
        // PrusaLink API to replace this?
        JSON_FIELD_STR("server", LWIP_VERSION_STRING) JSON_COMMA;
        JSON_FIELD_FFIXED("nozzle_diameter", nozzle_diameter, 2) JSON_COMMA;
        JSON_FIELD_STR("text", "PrusaLink") JSON_COMMA;
        JSON_FIELD_STR("hostname", hostname) JSON_COMMA;
        JSON_FIELD_OBJ("capabilities");
            JSON_FIELD_BOOL("upload-by-put", true);
        JSON_OBJ_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

JsonResult get_info(size_t resume_point, JsonOutput &output) {
    char hostname[HOSTNAME_LEN + 1];
    netdev_get_hostname(netdev_get_active_id(), hostname, sizeof hostname);
    float nozzle_diameter = config_store().get_nozzle_diameter(0);
    auto mmu2_enabled =
#if HAS_MMU2()
        config_store().mmu2_enabled.get();
#else
        false;
#endif
    serial_nr_t serial {};
    otp_get_serial_nr(serial);

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_FFIXED("nozzle_diameter", static_cast<double>(nozzle_diameter), 2) JSON_COMMA;
        JSON_FIELD_BOOL("mmu", mmu2_enabled) JSON_COMMA;
        JSON_FIELD_STR("serial", serial.begin()) JSON_COMMA;
        JSON_FIELD_STR("hostname", hostname) JSON_COMMA;
        JSON_FIELD_INT("min_extrusion_temp", EXTRUDE_MINTEMP);
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

JsonResult get_job_octoprint(size_t resume_point, JsonOutput &output) {
    // Note about the marlin vars: It's true that on resumption we may get
    // different values. But they would still be reasonably "sane". If we eg.
    // finish a print and base what we include on previous version, we may
    // outdated values, but they are still there.
    auto &vars = marlin_vars();

    bool has_job = false;
    const char *state = "Unknown";

    switch (vars.print_state) {
    case State::Finishing_WaitIdle:
    case State::Finishing_UnloadFilament:
    case State::Finishing_ParkHead:
    case State::Printing:
    case State::PrintPreviewConfirmed:
    case State::PowerPanic_acFault:
    case State::SerialPrintInit:
        has_job = true;
        state = "Printing";
        break;
    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_XY_Measure:
#if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup:
#endif
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
        has_job = true;
        state = "CrashRecovery";
        break;
    case State::Pausing_Begin:
    case State::Pausing_Failed_Code:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
        has_job = true;
        state = "Pausing";
        break;
    case State::PowerPanic_AwaitingResume:
    case State::Paused:
        has_job = true;
        state = "Paused";
        break;
    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE:
    case State::PowerPanic_Resume:
        has_job = true;
        state = "Resuming";
        break;
    case State::Aborting_Begin:
    case State::Aborting_WaitIdle:
    case State::Aborting_UnloadFilament:
    case State::Aborting_ParkHead:
    case State::Aborting_Preview:
        has_job = true;
        state = "Cancelling";
        break;
    case State::Aborted:
    case State::Finished:
    case State::Exit:
    case State::Idle:
    case State::WaitGui:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
#endif
    case State::PrintInit:
        state = "Operational";
        break;
    case State::PrintPreviewQuestions:
        has_job = true;
        state = "Error";
        break;
    }

    // The states here are different (and kinda weird, but changing it is not worth it, given we want to implement
    // the new v1 api anyway), but we probably still want the new attention states?.
    auto link_state = printer_state::get_state(false);
    if (link_state == DeviceState::Attention) {
        state = "Error";
        has_job = true;
    }

    char lfn_buffer[FILE_NAME_MAX_LEN];
    char sfn_buffer[FILE_PATH_MAX_LEN];
    {
        auto lock = MarlinVarsLockGuard();
        marlin_vars().media_LFN.copy_to(lfn_buffer, sizeof(lfn_buffer), lock);
        marlin_vars().media_SFN_path.copy_to(sfn_buffer, sizeof(sfn_buffer), lock);
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_STR("state", state) JSON_COMMA;
        if (has_job) {
            JSON_FIELD_OBJ("job");
                JSON_FIELD_INT("estimatedPrintTime", vars.print_duration + vars.time_to_end) JSON_COMMA;
                JSON_FIELD_OBJ("file")
                    JSON_FIELD_STR("name", lfn_buffer) JSON_COMMA;
                    JSON_FIELD_STR("path", sfn_buffer) JSON_COMMA;
                    JSON_FIELD_STR("display", lfn_buffer);
                JSON_OBJ_END;
            JSON_OBJ_END JSON_COMMA;
            JSON_FIELD_OBJ("progress");
                //Send only valid time_to_end value
                if (vars.time_to_end != TIME_TO_END_INVALID) {
                    JSON_FIELD_INT("printTimeLeft", vars.time_to_end) JSON_COMMA;
                }
                JSON_FIELD_FFIXED("completion", ((float)vars.sd_percent_done / 100.0f), 2) JSON_COMMA;
                JSON_FIELD_INT("printTime", vars.print_duration);
            JSON_OBJ_END;
        } else {
            JSON_CONTROL("\"job\": null,\"progress\": null");
        }
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

json::JsonResult get_job_v1(size_t resume_point, json::JsonOutput &output) {
    uint32_t time_to_end = marlin_vars().time_to_end;
    const char *state = "ERROR";
    auto link_state = printer_state::get_state(false);
    switch (link_state) {
    case printer_state::DeviceState::Printing:
        state = "PRINTING";
        break;
    case printer_state::DeviceState::Paused:
    case printer_state::DeviceState::Attention:
        state = "PAUSED";
        break;
    case printer_state::DeviceState::Finished:
        state = "FINISHED";
        break;
    case printer_state::DeviceState::Stopped:
        state = "STOPPED";
        break;
    default:
        state = "ERROR";
        break;
    }
    char filename[FILE_NAME_MAX_LEN];
    char sfn_path[FILE_PATH_MAX_LEN];
    {
        auto lock = MarlinVarsLockGuard();
        marlin_vars().media_LFN.copy_to(filename, sizeof(filename), lock);
        marlin_vars().media_SFN_path.copy_to(sfn_path, sizeof(sfn_path), lock);
    }

    bool has_stat { true };
    struct stat st;
    if (stat(sfn_path, &st) != 0) {
        // This should not happen in practice, but if it did we still want to handle
        // it gracefully
        has_stat = false;
    }

    JSONIFY_STR(sfn_path);

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_INT("id", marlin_vars().job_id) JSON_COMMA;
        JSON_FIELD_STR("state", state) JSON_COMMA;
        JSON_FIELD_FFIXED("progress", ((float)marlin_vars().sd_percent_done), 2) JSON_COMMA;
        if (time_to_end != TIME_TO_END_INVALID) {
            JSON_FIELD_INT("time_remaining", time_to_end) JSON_COMMA;
        }
        JSON_FIELD_INT("time_printing", marlin_vars().print_duration) JSON_COMMA;
        JSON_FIELD_OBJ("file");
            JSON_FIELD_OBJ("refs");
                JSON_CUSTOM("\"icon\":\"/thumb/s%s\",", sfn_path_escaped);
                JSON_CUSTOM("\"thumbnail\":\"/thumb/l%s\",", sfn_path_escaped);
                JSON_FIELD_STR("download", sfn_path);
            JSON_OBJ_END JSON_COMMA;
            JSON_FIELD_STR("name", basename_b(sfn_path)) JSON_COMMA;
            JSON_FIELD_STR("display_name", filename) JSON_COMMA;
            // Note: This modifies the buffer, so it has to be done after all other
            // uses of it above!!
            dirname(sfn_path);
            JSON_FIELD_STR("path", sfn_path);
            if (has_stat) {
                JSON_COMMA JSON_FIELD_INT_G(has_stat, "size", st.st_size) JSON_COMMA;
                JSON_FIELD_INT_G(has_stat, "m_timestamp", st.st_mtime);
            }
        JSON_OBJ_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

namespace {

    bool usb_available() {
        bool available = false;
        // ideally we would use something more lightweight, like stat()
        // but fatfs doesn't support calling it on root and from it's
        // perspective /usb is root
        if (DIR *dir = opendir("/usb"); dir != nullptr) {
            available = true;
            closedir(dir);
        }
        return available;
    }

} // namespace

JsonResult get_storage(size_t resume_point, JsonOutput &output) {
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
        JSON_OBJ_START
            JSON_FIELD_ARR("storage_list");
                JSON_OBJ_START
                    JSON_FIELD_STR("path", "/usb/") JSON_COMMA;
                    JSON_FIELD_STR("name", "usb") JSON_COMMA;
                    JSON_FIELD_STR("type", "USB") JSON_COMMA;
                    JSON_FIELD_BOOL("read_only", false) JSON_COMMA;
                    JSON_FIELD_BOOL("available", usb_available());
                JSON_OBJ_END;
            JSON_ARR_END;
        JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

} // namespace nhttp::link_content
