#include "render.hpp"

#include <segmented_json_macros.h>
#include <eeprom.h>
#include <lfn.h>
#include <filename_type.hpp>
#include <gcode_file.h>
#include <basename.h>
#include <timing.h>

#include <cassert>
#include <cstring>

using json::JsonOutput;
using json::JsonResult;
using std::get_if;
using std::make_tuple;
using std::min;
using std::move;
using std::nullopt;
using std::optional;
using std::tuple;
using std::visit;
using transfers::Monitor;

#define JSON_MAC(NAME, VAL) JSON_FIELD_STR_FORMAT(NAME, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", VAL[0], VAL[1], VAL[2], VAL[3], VAL[4], VAL[5])
#define JSON_IP(NAME, VAL)  JSON_FIELD_STR_FORMAT(NAME, "%hhu.%hhu.%hhu.%hhu", VAL[0], VAL[1], VAL[2], VAL[3])

namespace connect_client {

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
        case Printer::DeviceState::Ready:
            return "READY";
        case Printer::DeviceState::Error:
            return "ERROR";
        case Printer::DeviceState::Busy:
            return "BUSY";
        case Printer::DeviceState::Attention:
            return "ATTENTION";
        case Printer::DeviceState::Unknown:
        default:
            return "UNKNOWN";
        }
    }

    bool is_printing(Printer::DeviceState state) {
        switch (state) {
        case Printer::DeviceState::Printing:
        case Printer::DeviceState::Paused:
        case Printer::DeviceState::Attention:
            return true;
        default:
            return false;
        }
    }

    std::optional<transfers::Monitor::Status> get_transfer_status(size_t resume_point, const RenderState &state) {
        if (state.transfer_id.has_value()) {
            // If we've seen a transfer info previously, allow using a stale one to continue there.
            auto transfer_status = Monitor::instance.status(resume_point != 0);
            if (transfer_status.has_value() && transfer_status->id != state.transfer_id) {
                // But if the ID changed mid-report, bail out.
                return nullopt;
            }
            return transfer_status;
        } else {
            return nullopt;
        }
    }

    JsonResult render_msg(size_t resume_point, JsonOutput &output, const RenderState &state, const SendTelemetry &telemetry) {
        const auto params = state.printer.params();
        const bool printing = is_printing(params.state);

        const uint32_t current_fingerprint = params.telemetry_fingerprint(!printing);

        const optional<Monitor::Status> transfer_status = get_transfer_status(resume_point, state);

        // Note:
        // We don't adhere to the best practice of JSON renderers, that we
        // prepare everything up-front, store it and then render it. That's
        // because we don't want to store the copy of the structure while
        // sending, to be able to reuse the stack space.
        //
        // This isn't a big issue because:
        // * We don't call the printer.renew() / marlin_update_vars() in
        //   between, so the values _should_ be the same.
        // * The only way it can concievably change if it changes is to go from
        //   not changed -> changed telemetry. If it happens before entering the
        //   update_telemetry block, we just enter it. If it happens after, it
        //   has no effect (it's been already skipped).
        const bool update_telemetry = state.telemetry_changes.set_hash(current_fingerprint);
        // Keep the indentation of the JSON in here!
        // clang-format off
        JSON_START;
        JSON_OBJ_START;
            if (!telemetry.empty) {
                if (transfer_status.has_value()) {
                    // We use the guard-versions here, because we re-acquire
                    // the status on each resume of this "coroutine". In the
                    // very rare case the transfer ends and a new one starts in
                    // between, it might go away and we need to abort this
                    // attempt (we'll retry later on).
                    //
                    // To minimize the risk, we place these first.
                    //
                    // And yes, we need the guard on each one, because we can
                    // resume at each and every of these fields.
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_id", transfer_status->id) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_transferred", transfer_status->transferred) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_time_remaining", transfer_status->time_remaining_estimate()) JSON_COMMA;
                    JSON_FIELD_FFIXED_G(transfer_status.has_value(), "transfer_progress", transfer_status->progress_estimate() * 100.0, 1) JSON_COMMA;
                }

                // These are not included in the fingerprint as they are changing a lot.
                if (printing) {
                    JSON_FIELD_INT("time_printing", params.print_duration) JSON_COMMA;
                    JSON_FIELD_INT("time_remaining", params.time_to_end) JSON_COMMA;
                    JSON_FIELD_INT("progress", params.progress_percent) JSON_COMMA;
                }

                if (update_telemetry) {
                    JSON_FIELD_FFIXED("temp_nozzle", params.temp_nozzle, 1) JSON_COMMA;
                    JSON_FIELD_FFIXED("temp_bed", params.temp_bed, 1) JSON_COMMA;
                    JSON_FIELD_FFIXED("target_nozzle", params.target_nozzle, 1) JSON_COMMA;
                    JSON_FIELD_FFIXED("target_bed", params.target_bed, 1) JSON_COMMA;
                    JSON_FIELD_INT("speed", params.print_speed) JSON_COMMA;
                    JSON_FIELD_INT("flow", params.flow_factor) JSON_COMMA;
                    if (params.material != nullptr) {
                        JSON_FIELD_STR("material", params.material) JSON_COMMA;
                    }
                    if (!printing) {
                        // To avoid spamming the DB, connect doesn't want positions during printing
                        JSON_FIELD_FFIXED("axis_x", params.pos[Printer::X_AXIS_POS], 2) JSON_COMMA;
                        JSON_FIELD_FFIXED("axis_y", params.pos[Printer::Y_AXIS_POS], 2) JSON_COMMA;
                    }
                    JSON_FIELD_FFIXED("axis_z", params.pos[Printer::Z_AXIS_POS], 2) JSON_COMMA;
                    if (printing) {
                        JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
                        JSON_FIELD_INT("fan_extruder", params.heatbreak_fan_rpm) JSON_COMMA;
                        JSON_FIELD_INT("fan_print", params.print_fan_rpm) JSON_COMMA;
                        JSON_FIELD_FFIXED("filament", params.filament_used, 1) JSON_COMMA;
                    }
                }

                if (state.background_command_id.has_value()) {
                    JSON_FIELD_INT("command_id", *state.background_command_id) JSON_COMMA;
                }

                // State is sent always, first because it seems important, but
                // also, we want something that doesn't have the final comma on
                // it.
                JSON_FIELD_STR("state", to_str(params.state));
            }
        JSON_OBJ_END;
        JSON_END;
        // clang-format on
    }

    JsonResult render_msg(size_t resume_point, JsonOutput &output, RenderState &state, const Event &event) {
        const auto params = state.printer.params();
        const auto &info = state.printer.printer_info();
        const bool has_extra = (event.type != EventType::Accepted) && (event.type != EventType::Rejected);
        const bool printing = is_printing(params.state);

        const char *reject_with = nullptr;
        Printer::NetCreds creds = {};
        if (event.type == EventType::Info) {
            // Technically, it would be better to store this as part of
            // the render state. But that would be a bit wasteful, so
            // we do it here in a "late" fasion. At worst, we would get
            // the api key and ssid from two different times, but they
            // are not directly related to each other anyway.
            //
            // Prepare the creds here, before the magical switch hidden in
            // JSON_START... Otherwise it could be skipped on further
            // runs/resumes.
            creds = state.printer.net_creds();
        }

        if (event.type == EventType::JobInfo && (!printing || event.job_id != params.job_id)) {
            // Can't send a job info when not printing, refuse instead.
            //
            // Can't provide historic/future jobs.
            reject_with = printing ? "Job ID doesn't match" : "No job in progress";
        }

        if (event.type == EventType::FileInfo && !state.has_stat && !state.file_extra.renderer.holds_alternative<DirRenderer>()) {
            // The file probably doesn't exist or something
            // Exception for /usb, as that one doesn't have stat even though it exists.
            reject_with = "File not found";
        }

        const optional<Monitor::Status> transfer_status = event.type == EventType::TransferInfo ? get_transfer_status(resume_point, state) : nullopt;

        if (reject_with != nullptr) {
            // The fact we can render in multiple steps doesn't matter, we would
            // descend into here every time and resume the Rejected event.
            Event rejected(event);
            rejected.type = EventType::Rejected;
            rejected.reason = reject_with;
            return render_msg(resume_point, output, state, rejected);
        }

        // Keep the indentation of the JSON in here!
        // clang-format off
        JSON_START;
        JSON_OBJ_START;
            if (has_extra && printing) {
                JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
            }

            if (event.reason != nullptr) {
                JSON_FIELD_STR("reason", event.reason) JSON_COMMA;
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
                    JSON_FIELD_STR("fingerprint", info.fingerprint) JSON_COMMA;
                    if (strlen(creds.pl_password) > 0) {
                        JSON_FIELD_STR("api_key", creds.pl_password) JSON_COMMA;
                    }
                    JSON_FIELD_ARR("storages");
                    if (params.has_usb) {
                        JSON_OBJ_START;
                            // TODO: We may want to send a bit more info, just
                            //   for the user comfort, in particular:
                            // * Number of files directly under the root
                            // * Sizes (total/free/...)
                            // * Name of the filesystem if it is set/known.
                            JSON_FIELD_STR("mountpoint", "/usb") JSON_COMMA;
                            JSON_FIELD_STR("type", "USB") JSON_COMMA;
                            JSON_FIELD_BOOL("ro", false) JSON_COMMA;
                            if (event.info_rescan_files) {
                                JSON_FIELD_BOOL("rescan", true) JSON_COMMA;
                            }
                            JSON_FIELD_INT("free_space", params.usb_space_free) JSON_COMMA;
                            JSON_FIELD_BOOL("is_sfn", true);
                        JSON_OBJ_END;
                    }
                    JSON_ARR_END JSON_COMMA;
                    JSON_FIELD_OBJ("network_info");
                    if (state.lan.has_value()) {
                        JSON_MAC("lan_mac", state.lan->mac) JSON_COMMA;
                        JSON_IP("lan_ipv4", state.lan->ip);
                    }
                    if (state.lan.has_value() && state.wifi.has_value()) {
                        // Why oh why can't json accept a trailing comma :-(
                        JSON_COMMA;
                    }
                    if (state.wifi.has_value()) {
                        if (strlen(creds.ssid) > 0) {
                            JSON_FIELD_STR("wifi_ssid", creds.ssid) JSON_COMMA;
                        }
                        JSON_MAC("wifi_mac", state.wifi->mac) JSON_COMMA;
                        JSON_IP("wifi_ipv4", state.wifi->ip);
                    }
                    JSON_OBJ_END;
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::JobInfo) {
                JSON_FIELD_OBJ("data");
                    // The JobInfo doesn't claim the buffer, so we get it to store the path.
                    assert(params.job_path != nullptr);
                    if (state.has_stat) {
                        JSON_FIELD_INT("size", state.st.st_size) JSON_COMMA;
                        JSON_FIELD_INT("m_timestamp", state.st.st_mtime) JSON_COMMA;
                    }
                    JSON_FIELD_STR("display_name", params.job_lfn != nullptr ? params.job_lfn : basename_b(params.job_path)) JSON_COMMA;
                    JSON_FIELD_STR("path", params.job_path);
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::FileInfo) {
                JSON_FIELD_OBJ("data");
                    // Note: This chunk might or might not render anything.
                    //
                    // * In theory, it can be EmptyRenderer (though that should not happen in practice?)
                    // * In case of the PreviewRenderer, it could be that it is
                    //   not a gcode at all or doesn't contain the preview.
                    //
                    // For that reason, the renderer is responsible for
                    // rendering a trailing comma if it outputs anything at
                    // all.
                    JSON_CHUNK(state.file_extra.renderer);
                    if (state.has_stat) {
                        // has_stat might be off in case of /usb, that one acts
                        // "weird", as it is root of the FS.
                        JSON_FIELD_INT("size", state.st.st_size) JSON_COMMA;
                        JSON_FIELD_INT("m_timestamp", state.st.st_mtime) JSON_COMMA;
                    }
                    // Warning: the path->name() is there (hidden) for FileInfo
                    // but _not_ for JobInfo. Do not just copy that into that
                    // part!
                    JSON_FIELD_STR("display_name", event.path->name()) JSON_COMMA;
                    JSON_FIELD_STR("type", state.file_extra.renderer.holds_alternative<DirRenderer>() ? "FOLDER" : file_type_by_ext(event.path->path())) JSON_COMMA;
                    JSON_FIELD_STR("path", event.path->path());
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::TransferInfo) {
                JSON_FIELD_OBJ("data");
                if (transfer_status.has_value()) {
                    // Warning: The transfer_status was observed to have a
                    // value. But as we don't want to copy to the render state,
                    // we re-acquire it at every resume of this "coroutine".
                    //
                    // And it may become nullopt at that point (because the
                    // transfer has changed in between and we don't want to mix
                    // two inconsistent values).
                    //
                    // Therefore, we use the guards here ‒ in the very rare
                    // occasion (one transfer would have to end and another
                    // _start_ between the callS), we would just abort and
                    // hopefully retry / get asked to retry later on.
                    //
                    // And we really do need the guard on each one, because we
                    // can resume at each spot.
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_id", transfer_status->id) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "size", transfer_status->expected) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transferred", transfer_status->transferred) JSON_COMMA;
                    JSON_FIELD_FFIXED_G(transfer_status.has_value(), "progress", transfer_status->progress_estimate() * 100.0, 1) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "time_remaining", transfer_status->time_remaining_estimate()) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "time_transferring", ticks_s() - transfer_status->start) JSON_COMMA;
                    // Note: This works, because destination cannot go from non null to null
                    // (if one transfer ends and another starts mid report, we bail out)
                    if (transfer_status->destination) {
                        JSON_FIELD_STR_G(transfer_status.has_value(), "path", transfer_status->destination) JSON_COMMA;
                    }
                    JSON_FIELD_STR_G(transfer_status.has_value(), "type", to_str(transfer_status->type));
                } else {
                    JSON_FIELD_STR("type", "NO_TRANSFER");
                }
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::TransferStopped || event.type == EventType::TransferAborted || event.type == EventType::TransferFinished) {
                JSON_FIELD_OBJ("data");
                    assert(event.transfer_id.has_value());
                    JSON_FIELD_INT("transfer_id", *event.transfer_id);
                JSON_OBJ_END JSON_COMMA;
            }

            JSON_FIELD_STR("state", to_str(params.state)) JSON_COMMA;
            if (event.command_id.has_value()) {
                JSON_FIELD_INT("command_id", *event.command_id) JSON_COMMA;
            }
            if (state.transfer_id.has_value()) {
                // In case of the TRANSFER_ID event, this is actually duplicated.
                JSON_FIELD_INT("transfer_id", *state.transfer_id) JSON_COMMA;
            }
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

    off_t child_size(const char *base_path, const char *child_name) {
        char path_buf[FILE_PATH_BUFFER_LEN];
        int formatted = snprintf(path_buf, sizeof path_buf, "%s/%s", base_path, child_name);
        // Name didn't fit. That, in theory, should not happen, but better safe than sorry...
        if (formatted >= FILE_NAME_BUFFER_LEN - 1) {
            return -1;
        }
        struct stat st = {};
        if (stat(path_buf, &st) == 0) {
            return st.st_size;
        } else {
            return -1;
        }
    }

    enum class MetaFilter {
        Ignore,
        String,
        Int,
        Float,
        Bool,
    };

    struct MetaRecord {
        const char *name;
        MetaFilter filter;
    };

    // TODO: We probably can come up with some way of not storing the long
    // strings in here and save some flash size with maybe CRCs of the strings?
    static constexpr MetaRecord meta_records[] = {
        { "estimated printing time (normal mode)", MetaFilter::String },
        { "filament cost", MetaFilter::Float },
        { "filament used [mm]", MetaFilter::Float },
        { "filament used [cm3]", MetaFilter::Float },
        { "filament used [mm3]", MetaFilter::Float },
        { "filament used [g]", MetaFilter::Float },
        { "filament used [m]", MetaFilter::Float },
        { "bed_temperature", MetaFilter::Int },
        { "brim_width", MetaFilter::Int },
        { "filament_type", MetaFilter::String },
        // Yes, really, a string, because it contains the % sign.
        { "fill_density", MetaFilter::String },
        { "layer_height", MetaFilter::Float },
        { "nozzle_diameter", MetaFilter::Float },
        { "printer_model", MetaFilter::String },
        { "temperature", MetaFilter::Int },
        // Note: These two should actually be Bools. But it seems the server is
        // currently expecting 0/1, the gcode also contains 0/1, so we adhere
        // to the rest because of compatibility.
        { "ironing", MetaFilter::Int },
        { "support_material", MetaFilter::Int },
        { "max_layer_z", MetaFilter::Float },
    };

    MetaFilter meta_filter(const char *name) {
        for (size_t i = 0; i < sizeof meta_records / sizeof *meta_records; i++) {
            if (strcmp(name, meta_records[i].name) == 0) {
                return meta_records[i].filter;
            }
        }

        return MetaFilter::Ignore;
    }

    const char *display_name(const dirent *ent) {
#ifdef UNITTESTS
        return ent->d_name;
#else
        if (ent->lfn != nullptr) {
            return ent->lfn;
        } else {
            // Fatfs without long file name...
            return ent->d_name;
        }
#endif
    }
}

PreviewRenderer::PreviewRenderer(FILE *f)
    // Ask for anything bigger than 16x16 (at least 17x17).
    : decoder(f, 17, 17, false, true) {}

tuple<JsonResult, size_t> PreviewRenderer::render(uint8_t *buffer, size_t buffer_size) {
    constexpr const char *intro = "\"preview\":\"";
    constexpr const char *outro = "\",";
    constexpr size_t intro_len = strlen(intro);
    // Ending quote and comma
    constexpr size_t outro_len = strlen(outro);
    // Don't bother with too small buffers to make the code easier. Extra char
    // for trying out there's some preview in there.
    constexpr size_t min_len = intro_len + outro_len + 1;

    if (buffer_size < min_len) {
        // Will be retried next time with bigger buffer.
        return make_tuple(JsonResult::BufferTooSmall, 0);
    }

    size_t written = 0;

    if (!started) {
        // It's OK to write into the buffer even if we would claim not to have
        // written there later on.
        memcpy(buffer, intro, intro_len);
        written += intro_len;
        buffer += intro_len;
        buffer_size -= intro_len;
    }

    const size_t available = buffer_size - outro_len;
    assert(available > 0);
    int decoded = decoder.Read(reinterpret_cast<char *>(buffer), available);
    if (decoded == -1) {
        // -1 signals error.
        if (started) {
            // At least terminate the field/string, so we don't destroy the
            // whole JSON, even if the preview data is truncated.
            memcpy(buffer, outro, outro_len);
            return make_tuple(JsonResult::Complete, outro_len);
        } else {
            return make_tuple(JsonResult::Complete, 0);
        }
    }

    if (decoded == 0 && !started) {
        // No preview -> just say we didn't do anything at all.
        return make_tuple(JsonResult::Complete, 0);
    }

    started = true;
    written += decoded;
    buffer += decoded;
    buffer_size -= decoded;

    JsonResult result = JsonResult::Incomplete;

    if (decoded < static_cast<int>(available)) {
        // This is the end!
        result = JsonResult::Complete;
        memcpy(buffer, outro, outro_len);
        written += outro_len;
    }

    return make_tuple(result, written);
}

tuple<JsonResult, size_t> GcodeMetaRenderer::render(uint8_t *buffer, size_t buffer_size) {
    // Special case, we "start" not an the beginning, but some amount from the
    // end ‒ we don't want to read through all the long gcode in the middle.
    // Of course, if the file is shorter, we just start from the beginning instead.
    if (resume_position == 0) {
        if (fseek(f, -f_gcode_search_last_x_bytes, SEEK_END) != 0) {
            fseek(f, 0, SEEK_SET);
        }
    } else {
        // The last one didn't fit, so return before that one and retry.
        // (we are fine not checking the errors; they'd be result of maybe
        // removed USB drive or such and that'll just explode a bit lower in
        // the code).
        fseek(f, resume_position, SEEK_SET);
    }

    char name_buffer[64];
    char value_buffer[32];

    size_t buffer_size_rest = buffer_size;
    // We are reusing the JsonOutput here, but not using the resume point
    // feature of it. We still need to provide the variable to it, though.
    size_t resume_point = 0;
    JsonOutput output(buffer, buffer_size_rest, resume_point);
    // The output does track how much it used. But we need to track it in the
    // whole fields resolution, including commas ‒ otherwise the code would
    // become significantly more complicated by a comma being able to overflow
    // to the next buffer of data.
    size_t pos = 0;

    while (f_gcode_get_next_comment_assignment(f, name_buffer, sizeof name_buffer, value_buffer, sizeof value_buffer)) {
        JsonResult result = JsonResult::Complete;

        const auto filter = meta_filter(name_buffer);
        switch (filter) {
        case MetaFilter::Ignore:
            goto SKIP;

        case MetaFilter::String:
            result = output.output_field_str(0, name_buffer, value_buffer);
            break;

        case MetaFilter::Float: {
            char *end = nullptr;
            double v = strtod(value_buffer, &end);
            if (end != nullptr && *end != '\0') {
                goto SKIP;
            }

            result = output.output_field_float_fixed(0, name_buffer, v, 2);
            break;
        }

        case MetaFilter::Int:
        case MetaFilter::Bool: {
            char *end = nullptr;
            long v = strtol(value_buffer, &end, 10);
            if (end != nullptr && *end != '\0') {
                // Not really an int there.
                goto SKIP;
            }

            if (filter == MetaFilter::Int) {
                result = output.output_field_int(0, name_buffer, v);
            } else {
                // The gcode encodes bools as 0/1, JSON has True and False.
                result = output.output_field_bool(0, name_buffer, v);
            }
            break;
        }
        }

        if (result == JsonResult::Complete) {
            result = output.output(0, ",");
        }

        switch (result) {
        case JsonResult::Complete:
            // Successfully put into the buffer.
            break;
        case JsonResult::Abort:
            // We use only the primitive output functions and they are not
            // capable of returning Abort.
            assert(0);
            break;
        case JsonResult::Incomplete:
        case JsonResult::BufferTooSmall:
            // The primitive functions get "confused" a little bit by always
            // using the resume point of 0 and reports BufferTooSmall. But
            // that's fine, we don't really need to make that distinction here.
            return make_tuple(JsonResult::Incomplete, pos);
        }

    SKIP:
        // Adjust these only after the whole field, including comma. Not in
        // case it doesn't fit.
        resume_position = ftell(f);
        pos = buffer_size - buffer_size_rest;
    }

    return make_tuple(JsonResult::Complete, pos);
}

DirRenderer::DirRenderer(const char *base_path, unique_dir_ptr dir)
    : JsonRenderer(DirState { move(dir), base_path }) {}

JsonResult DirRenderer::renderState(size_t resume_point, json::JsonOutput &output, DirState &state) const {
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_FIELD_ARR("children");
    while (state.dir.get() && (state.ent = readdir(state.dir.get()))) {
        state.child_cnt ++;

        if (!state.first) {
            JSON_COMMA;
        } else {
            state.first = false;
        }

        JSON_OBJ_START;
            JSON_FIELD_STR("name", state.ent->d_name) JSON_COMMA;
            JSON_FIELD_STR("display_name", display_name(state.ent)) JSON_COMMA;
            JSON_FIELD_INT("size", child_size(state.base_path, state.ent->d_name)) JSON_COMMA;
            // We assume USB is not read only for us.
            JSON_FIELD_BOOL("ro", false) JSON_COMMA;
            JSON_FIELD_STR("type", file_type(state.ent));
        JSON_OBJ_END;
    }
    JSON_ARR_END JSON_COMMA;
    JSON_FIELD_INT("file_count", state.child_cnt) JSON_COMMA;
    JSON_END;
    // clang-format on
}

FileExtra::FileExtra(unique_file_ptr file)
    : file(move(file))
    , renderer(move(GcodeExtra(PreviewRenderer(this->file.get()), GcodeMetaRenderer(this->file.get())))) {}

FileExtra::FileExtra(const char *base_path, unique_dir_ptr dir)
    : renderer(move(DirRenderer(base_path, move(dir)))) {}

RenderState::RenderState(const Printer &printer, const Action &action, Tracked &telemetry_changes, optional<CommandId> background_command_id)
    : printer(printer)
    , action(action)
    , telemetry_changes(telemetry_changes)
    , lan(printer.net_info(Printer::Iface::Ethernet))
    , wifi(printer.net_info(Printer::Iface::Wifi))
    , transfer_id(Monitor::instance.id())
    , background_command_id(background_command_id) {
    memset(&st, 0, sizeof st);

    if (const auto *event = get_if<Event>(&action); event != nullptr) {
        const char *path = nullptr;
        const auto params = printer.params();
        bool error = false;

        switch (event->type) {
        case EventType::JobInfo:
            if (is_printing(params.state)) {
                path = params.job_path;
            }
            break;
        case EventType::FileInfo: {
            assert(event->path.has_value());
            SharedPath spath = event->path.value();
            path = spath.path();

            if (unique_dir_ptr d(opendir(path)); d.get() != nullptr) {
                file_extra = FileExtra(path, move(d));
            } else if (unique_file_ptr f(fopen(path, "r")); f.get() != nullptr) {
                file_extra = FileExtra(move(f));
            } else {
                error = true;
            }
            // We are being rude here a bit. While the event is const, we modify the shared buffer. Nevertheless:
            // * The shared buffer is not shared into other threads, so nobody
            //   is reading it at the same time as we are writing into it.
            // * If this is ever called multiple times (it can be, if the same
            //   event needs to be resent), it results into the same values
            //   there.
            get_LFN(spath.name(), FILE_NAME_BUFFER_LEN, spath.path());
            break;
        }
        default:;
        }

        if (!error && path != nullptr && stat(path, &st) == 0) {
            has_stat = true;
        }
    }
}

JsonResult Renderer::renderState(size_t resume_point, JsonOutput &output, RenderState &state) const {
    return visit([&](auto action) -> JsonResult {
        return render_msg(resume_point, output, state, action);
    },
        state.action);
}

}
