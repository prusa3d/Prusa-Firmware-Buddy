#include "gcode_info_scan.hpp"

#include "gcode_info.hpp"
#include <scope_guard.hpp>
#include <common/async_job/async_job.hpp>

static AsyncJob gcode_scan_async_job;

LOG_COMPONENT_REF(MarlinServer);

static void gcode_info_scan_callback(AsyncJobExecutionControl &control) {
    AnyGcodeFormatReader gcode_info_file;
    auto &gcode_info = GCodeInfo::getInstance();

    if (!gcode_info.start_load(gcode_info_file)) {
        log_error(MarlinServer, "Media prefetch GCodeInfo: fail to open");
        return;
    }

    ScopeGuard exit_callback = [&] {
        gcode_info.end_load(gcode_info_file);
    };

    // Wait for gcode to be valid
    while (!gcode_info.check_valid_for_print(gcode_info_file)) {
        if (gcode_info.has_error()) {
            log_error(MarlinServer, "Media prefetch GCodeInfo: not valid: %s", gcode_info.error_str());
            return;
        }

        osDelay(500);

        if (control.is_discarded()) {
            return;
        }
    }

    log_info(MarlinServer, "Media prefetch GCodeInfo: loading");
    gcode_info.load(gcode_info_file);
}

namespace gcode_info_scan {

void start_scan() {
    gcode_scan_async_job.issue(&gcode_info_scan_callback);
}

void cancel_scan() {
    gcode_scan_async_job.discard();
}

} // namespace gcode_info_scan
