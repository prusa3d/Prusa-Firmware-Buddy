#include "transfer_renderer.h"

#include <segmented_json_macros.h>
#include <filepath_operation.h>
#include <lfn.h>

using namespace json;
using namespace http;
using namespace transfers;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace nhttp::handler {

JsonResult TransferRenderer::renderState(size_t resume_point, json::JsonOutput &output, TransferState &state) const {
    if (api == APIVersion::Octoprint) {
        return renderStateOctoprint(resume_point, output, state);
    } else {
        return renderStateV1(resume_point, output, state);
    }
}

JsonResult TransferRenderer::renderStateV1(size_t resume_point, json::JsonOutput &output, TransferState &state) const {
    // Note: We allow stale, because we already checked there was a trasnfer running
    // in prusa_link_api, so even if it ended, we want to report it.
    auto transfer_status = Monitor::instance.status(true);
    if (transfer_status.has_value() && transfer_status->id != state.transfer_id) {
        // if transfer changes mid report, bail out
        transfer_status.reset();
    }

    // FIXME: old POST transfer does not know the path upfront
    //  this might be a problem if old Slicer would be uploading.
    //  But right now Link does not seems to do anything with the
    //  transfer information anyway, so maybe not a big deal?
    char filepath[FILE_PATH_BUFFER_LEN];
    char lfn[FILE_NAME_BUFFER_LEN];
    if (transfer_status.has_value() && transfer_status->destination) {
        strlcpy(filepath, transfer_status->destination, sizeof(filepath));
        get_SFN_path(filepath);
        get_LFN(lfn, sizeof(lfn), filepath);
        dirname(filepath);
    } else {
        filepath[0] = '\0';
        lfn[0] = '\0';
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
        JSON_OBJ_START
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "type", "%s", to_str(transfer_status->type)) JSON_COMMA;
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "display_name", "%s", lfn) JSON_COMMA;
            JSON_FIELD_STR_437_G(transfer_status.has_value(), "path", filepath) JSON_COMMA;
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "size", "%d", transfer_status->expected) JSON_COMMA;
            JSON_FIELD_FFIXED_G(transfer_status.has_value(), "progress", transfer_status->progress_estimate(), 2) JSON_COMMA;
            JSON_FIELD_INT_G(transfer_status.has_value(), "transferred", transfer_status->download_progress.get_valid_size()) JSON_COMMA;
            JSON_FIELD_INT_G(transfer_status.has_value(), "time_remaining", transfer_status->time_remaining_estimate()) JSON_COMMA;
            JSON_FIELD_INT_G(transfer_status.has_value(), "time_transferring", transfer_status->time_transferring()) JSON_COMMA;
            JSON_FIELD_BOOL_G(transfer_status.has_value(), "to_print", transfer_status->print_after_upload);
        JSON_OBJ_END
    JSON_END;
    // clang-format on
}

JsonResult TransferRenderer::renderStateOctoprint(size_t resume_point, json::JsonOutput &output, TransferState &state) const {
    // Note: We allow stale, because we already checked there was a trasnfer running
    // in prusa_link_api, so even if it ended, we want to report it.
    auto transfer_status = Monitor::instance.status(true);
    if (transfer_status.has_value() && transfer_status->id != state.transfer_id) {
        // if transfer changes mid report, bail out
        transfer_status.reset();
    }

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
        JSON_OBJ_START
            JSON_FIELD_STR("target", "usb") JSON_COMMA;
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "type", "%s", to_str(transfer_status->type)) JSON_COMMA;
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "size", "%d", transfer_status->expected) JSON_COMMA;
            //FIXME: Right now should be seconds from epoch start, would be much nicer
            // for us to make it seconds from print start and make the client compute the rest,
            // also it would solve some potential problems with NTP and time zones etc.
            JSON_FIELD_STR_FORMAT_G(transfer_status.has_value(), "start_time", "%d", time(nullptr) - (transfer_status->time_transferring())) JSON_COMMA;
            JSON_FIELD_BOOL_G(transfer_status.has_value(), "to_print", transfer_status->print_after_upload) JSON_COMMA;
            // Note: This works, because destination cannot go from non null to null
            // (if one transfer ends and another starts mid report, we bail out)
            if (transfer_status->destination) {
                JSON_FIELD_STR_G(transfer_status.has_value(), "destination", transfer_status->destination) JSON_COMMA;
            }
            JSON_FIELD_FFIXED_G(transfer_status.has_value(), "progress", transfer_status->progress_estimate(), 2) JSON_COMMA;
            JSON_FIELD_INT_G(transfer_status.has_value(), "remaining_time", transfer_status->time_remaining_estimate());
        JSON_OBJ_END
    JSON_END;
    // clang-format on
}
} // namespace nhttp::handler
