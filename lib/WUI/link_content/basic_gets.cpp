#include "basic_gets.h"
#include "filament.h" //get_selected_filament_name
#include "marlin_client.h"
#include "lwip/init.h"
#include "netdev.h"

#include <segmented_json_macros.h>

#include <dirent.h>
#include <cstring>
#include <cstdio>

using namespace json;
namespace nhttp::link_content {

JsonResult get_printer(size_t resume_point, JsonOutput &output) {
    // Note about the marlin vars: It's true that on resumption we may get
    // different values. But they would still be reasonably "sane". If we eg.
    // finish a print and base what we include on previous version, we may
    // outdated values, but they are still there.
    marlin_vars_t *vars = marlin_vars();
    const char *filament_material = get_selected_filament_name();

    bool operational = true;
    bool paused = false;
    bool printing = false;
    bool cancelling = false;
    bool pausing = false;
    bool ready = true;
    bool busy = false;
    bool error = false;
    const char *link_state = nullptr;

    marlin_update_vars(MARLIN_VAR_MSK_TEMP_ALL | MARLIN_VAR_MSK4(MARLIN_VAR_PRNSPEED, MARLIN_VAR_POS_Z, MARLIN_VAR_PRNSPEED, MARLIN_VAR_PRNSTATE));

    switch (vars->print_state) {
    case mpsCrashRecovery_Begin:
    case mpsCrashRecovery_Lifting:
    case mpsCrashRecovery_Retracting:
    case mpsCrashRecovery_XY_Measure:
    case mpsCrashRecovery_XY_HOME:
    case mpsCrashRecovery_Axis_NOK:
    case mpsCrashRecovery_Repeated_Crash:
    case mpsPowerPanic_acFault:
        link_state = "BUSY";
        busy = true;
        // Fall through
    case mpsPrinting:
        printing = true;
        ready = operational = false;
        link_state = "PRINTING";
        break;
    case mpsPowerPanic_AwaitingResume:
    case mpsPausing_Begin:
    case mpsPausing_Failed_Code:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        printing = pausing = paused = busy = true;
        ready = operational = false;
        link_state = "PAUSED";
        break;
    case mpsPaused:
        printing = paused = true;
        ready = operational = false;
        link_state = "PAUSED";
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead_XY:
    case mpsResuming_UnparkHead_ZE:
    case mpsPowerPanic_Resume:
        ready = operational = false;
        busy = printing = true;
        link_state = "PRINTING";
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        cancelling = busy = true;
        ready = operational = false;
        link_state = "BUSY";
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
        busy = true;
        ready = operational = false;
        link_state = "BUSY";
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsExit:
    case mpsIdle:
    case mpsWaitGui:
    case mpsPrintPreviewInit:
    case mpsPrintPreviewImage:
    case mpsPrintInit:
        break;
    case mpsPrintPreviewQuestions:
        // The "preview" is abused to ask questions about the filament and such.
        busy = printing = error = true;
        link_state = "ATTENTION";
        break;
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_OBJ("telemetry");
            JSON_FIELD_FFIXED("temp-bed", vars->temp_bed, 1) JSON_COMMA;
            JSON_FIELD_FFIXED("temp-nozzle", vars->temp_nozzle, 1) JSON_COMMA;
            JSON_FIELD_INT("print-speed", vars->print_speed) JSON_COMMA;
            // XYZE, mm
            JSON_FIELD_FFIXED("z-height", vars->curr_pos[2], 1) JSON_COMMA;
            JSON_FIELD_STR("material", filament_material);
        JSON_OBJ_END JSON_COMMA;

        JSON_FIELD_OBJ("temperature");
            JSON_FIELD_OBJ("tool0");
                JSON_FIELD_FFIXED("actual", vars->temp_nozzle, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("target", vars->target_nozzle, 1) JSON_COMMA;
                // Note: our own extension, because our printers sometimes display
                // different "target" temperature than what they heat towards.
                JSON_FIELD_FFIXED("display", vars->display_nozzle, 1) JSON_COMMA;
                JSON_FIELD_INT("offset", 0);
            JSON_OBJ_END JSON_COMMA;
            JSON_FIELD_OBJ("bed");
                JSON_FIELD_FFIXED("actual", vars->temp_bed, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("target", vars->target_bed, 1) JSON_COMMA;
                JSON_FIELD_INT("offset", 0);
            JSON_OBJ_END;
        JSON_OBJ_END JSON_COMMA;

        JSON_FIELD_OBJ("state")
            JSON_FIELD_STR("text", printing ? "Printing" : "Operational") JSON_COMMA;
            JSON_FIELD_OBJ("flags");
                if (link_state != nullptr) {
                    JSON_FIELD_STR("link_state", link_state) JSON_COMMA;
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
    char hostname[ETH_HOSTNAME_LEN + 1];
    netdev_get_hostname(netdev_get_active_id(), hostname, sizeof hostname);

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_STR("api", PL_VERSION_STRING) JSON_COMMA;
        // FIXME: The server version is probably bogus. But it's unclear what
        // the version is supposed to mean anyway. Waiting for the new
        // PrusaLink API to replace this?
        JSON_FIELD_STR("server", LWIP_VERSION_STRING) JSON_COMMA;
        JSON_FIELD_STR("text", "PrusaLink MINI") JSON_COMMA;
        JSON_FIELD_STR("hostname", hostname) JSON_COMMA;
        JSON_FIELD_OBJ("capabilities");
            JSON_FIELD_BOOL("upload-by-put", true);
        JSON_OBJ_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

JsonResult get_job(size_t resume_point, JsonOutput &output) {
    // Note about the marlin vars: It's true that on resumption we may get
    // different values. But they would still be reasonably "sane". If we eg.
    // finish a print and base what we include on previous version, we may
    // outdated values, but they are still there.
    marlin_vars_t *vars = marlin_vars();
    marlin_update_vars(MARLIN_VAR_MSK5(MARLIN_VAR_PRNSTATE, MARLIN_VAR_DURATION, MARLIN_VAR_TIMTOEND, MARLIN_VAR_FILEPATH, MARLIN_VAR_SD_PDONE));

    bool has_job = false;
    const char *state = "Unknown";

    switch (vars->print_state) {
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
    case mpsPrinting:
    case mpsPowerPanic_acFault:
        has_job = true;
        state = "Printing";
        break;
    case mpsCrashRecovery_Begin:
    case mpsCrashRecovery_Lifting:
    case mpsCrashRecovery_Retracting:
    case mpsCrashRecovery_XY_Measure:
    case mpsCrashRecovery_XY_HOME:
    case mpsCrashRecovery_Axis_NOK:
    case mpsCrashRecovery_Repeated_Crash:
        has_job = true;
        state = "CrashRecovery";
        break;
    case mpsPausing_Begin:
    case mpsPausing_Failed_Code:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        has_job = true;
        state = "Pausing";
        break;
    case mpsPowerPanic_AwaitingResume:
    case mpsPaused:
        has_job = true;
        state = "Paused";
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead_XY:
    case mpsResuming_UnparkHead_ZE:
    case mpsPowerPanic_Resume:
        has_job = true;
        state = "Resuming";
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        has_job = true;
        state = "Cancelling";
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsExit:
    case mpsIdle:
    case mpsWaitGui:
    case mpsPrintPreviewInit:
    case mpsPrintPreviewImage:
    case mpsPrintInit:
        state = "Operational";
        break;
    case mpsPrintPreviewQuestions:
        has_job = true;
        state = "Error";
        break;
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
        JSON_FIELD_STR("state", state) JSON_COMMA;
        if (has_job) {
            JSON_FIELD_OBJ("job");
                JSON_FIELD_INT("estimatedPrintTime", vars->print_duration + vars->time_to_end) JSON_COMMA;
                JSON_FIELD_OBJ("file")
                    JSON_FIELD_STR("name", vars->media_LFN) JSON_COMMA;
                    JSON_FIELD_STR("path", vars->media_SFN_path) JSON_COMMA;
                    JSON_FIELD_STR("display", vars->media_LFN);
                JSON_OBJ_END;
            JSON_OBJ_END JSON_COMMA;
            JSON_FIELD_OBJ("progress");
                JSON_FIELD_FFIXED("completion", ((float)vars->sd_percent_done / 100.0f), 2) JSON_COMMA;
                JSON_FIELD_INT("printTime", vars->print_duration) JSON_COMMA;
                JSON_FIELD_INT("printTimeLeft", vars->time_to_end);
            JSON_OBJ_END;
        } else {
            JSON_CONTROL("\"job\": null,\"progress\": null");
        }
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

}

JsonResult get_storage(size_t resume_point, JsonOutput &output) {
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
        JSON_OBJ_START
            JSON_FIELD_ARR("storage_list");
                JSON_OBJ_START
                    JSON_FIELD_STR("path", "/usb") JSON_COMMA;
                    JSON_FIELD_STR("type", "USB") JSON_COMMA;
                    JSON_FIELD_BOOL("read_only", false) JSON_COMMA;
                    JSON_FIELD_BOOL("available", usb_available());
                JSON_OBJ_END;
            JSON_ARR_END;
        JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

}
