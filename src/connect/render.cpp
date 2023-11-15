#include "render.hpp"
#include "printer_type.hpp"

#include <segmented_json_macros.h>
#include <lfn.h>
#include <filename_type.hpp>
#include <filepath_operation.h>
#include <timing.h>
#include <state/printer_state.hpp>
#include <transfers/transfer.hpp>

#include <cassert>
#include <cstring>

#include <marlin_server_shared.h>
#include <mbedtls/base64.h>

using json::JsonOutput;
using json::JsonResult;
using printer_state::DeviceState;
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

    bool is_printing(DeviceState state) {
        switch (state) {
        case DeviceState::Printing:
        case DeviceState::Paused:
        case DeviceState::Attention:
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
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_transferred", transfer_status->download_progress.get_valid_size()) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_time_remaining", transfer_status->time_remaining_estimate()) JSON_COMMA;
                    JSON_FIELD_FFIXED_G(transfer_status.has_value(), "transfer_progress", transfer_status->progress_estimate() * 100.0, 1) JSON_COMMA;
                }

                // These are not included in the fingerprint as they are changing a lot.
                if (printing) {
                    JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
                    JSON_FIELD_INT("time_printing", params.print_duration) JSON_COMMA;
                    if (params.time_to_end != marlin_server::TIME_TO_END_INVALID) {
                        JSON_FIELD_INT("time_remaining", params.time_to_end) JSON_COMMA;
                    }
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

        if (event.type == EventType::JobInfo && (!printing || event.job_id.value_or(params.job_id) != params.job_id)) {
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
                    JSON_FIELD_STR_FORMAT("printer_type", "%hhu.%hhu.%hhu", params.version.type, params.version.version, params.version.subversion) JSON_COMMA;
                    JSON_FIELD_STR("sn", info.serial_number.begin()) JSON_COMMA;
                    JSON_FIELD_BOOL("appendix", info.appendix) JSON_COMMA;
                    JSON_FIELD_STR("fingerprint", info.fingerprint) JSON_COMMA;
                    JSON_FIELD_FFIXED("nozzle_diameter", params.nozzle_diameter, 2) JSON_COMMA;
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
                            JSON_FIELD_BOOL("read_only", false) JSON_COMMA;
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
                    assert(params.job_path() != nullptr);
                    if (state.has_stat) {
                        JSON_FIELD_INT("size", state.st.st_size) JSON_COMMA;
                        JSON_FIELD_INT("m_timestamp", state.st.st_mtime) JSON_COMMA;
                    }
                    if (params.job_lfn() != nullptr) {
                        JSON_FIELD_STR("display_name", params.job_lfn());
                    } else {
                        JSON_FIELD_STR_437("display_name", basename_b(params.job_path()));
                    }
                    JSON_COMMA;
                    if (event.start_cmd_id.has_value()) {
                        JSON_FIELD_INT("start_cmd_id", *event.start_cmd_id) JSON_COMMA;
                    }
                    JSON_FIELD_STR_437("path", params.job_path());
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
                    JSON_FIELD_BOOL("read_only", state.read_only) JSON_COMMA;
                    // Warning: the path->name() is there (hidden) for FileInfo
                    // but _not_ for JobInfo. Do not just copy that into that
                    // part!
                    //
                    // XXX: Can the name be SFN?
                    JSON_FIELD_STR("display_name", event.path->name()) JSON_COMMA;
                    JSON_FIELD_STR("type", state.file_extra.renderer.holds_alternative<DirRenderer>() ? "FOLDER" : file_type_by_ext(event.path->path())) JSON_COMMA;
                    JSON_FIELD_STR_437("path", event.path->path());
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
                    JSON_FIELD_INT_G(transfer_status.has_value(), "size", transfer_status->expected) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transferred", transfer_status->download_progress.get_valid_size()) JSON_COMMA;
                    JSON_FIELD_FFIXED_G(transfer_status.has_value(), "progress", transfer_status->progress_estimate() * 100.0, 1) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "time_remaining", transfer_status->time_remaining_estimate()) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "time_transferring", transfer_status->time_transferring()) JSON_COMMA;
                    // Note: This works, because destination cannot go from non null to null
                    // (if one transfer ends and another starts mid report, we bail out)
                    if (transfer_status->destination) {
                        // FIXME: This one is problematic, part is SFN, part is LFN.
                        //
                        // For now, we consider it SFN (because that always produces valid utf8 at least), needs fix later on.
                        JSON_FIELD_STR_G(transfer_status.has_value(), "path", transfer_status->destination) JSON_COMMA;
                    }
                    if (event.start_cmd_id.has_value()) {
                        JSON_FIELD_INT("start_cmd_id", *event.start_cmd_id) JSON_COMMA;
                    }
                    JSON_FIELD_STR_G(transfer_status.has_value(), "type", to_str(transfer_status->type));
                } else {
                    JSON_FIELD_STR("type", "NO_TRANSFER");
                }
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::TransferStopped || event.type == EventType::TransferAborted || event.type == EventType::TransferFinished) {
                if (event.start_cmd_id.has_value()) {
                    JSON_FIELD_OBJ("data");
                        JSON_FIELD_INT("start_cmd_id", *event.start_cmd_id);
                    JSON_OBJ_END JSON_COMMA;
                }
            } else if (event.type == EventType::FileChanged) {
                // FIXME: This is just an educated guess, the exact protocol has not been decided yet.
                JSON_FIELD_OBJ("data");
                    if (params.has_usb) {
                        JSON_FIELD_INT("free_space", params.usb_space_free) JSON_COMMA;
                    }
                    if (event.incident == transfers::ChangedPath::Incident::Created || event.incident == transfers::ChangedPath::Incident::CreatedEarly) {
                        JSON_FIELD_STR_437("new_path", event.path->path()) JSON_COMMA;
                    } else if (event.incident == transfers::ChangedPath::Incident::Deleted) {
                        JSON_FIELD_STR_437("old_path", event.path->path()) JSON_COMMA;
                    } else /*Combined*/ {
                        JSON_FIELD_STR_437("new_path", event.path->path()) JSON_COMMA;
                        JSON_FIELD_BOOL("rescan", true) JSON_COMMA;
                    }
                    JSON_FIELD_OBJ("file")
                        if (state.has_stat) {
                            // has_stat might be off in case of /usb, that one acts
                            // "weird", as it is root of the FS.
                            JSON_FIELD_INT("size", state.st.st_size) JSON_COMMA;
                            JSON_FIELD_INT("m_timestamp", state.st.st_mtime) JSON_COMMA;
                        }
                        JSON_FIELD_STR("type", event.is_file ? file_type_by_ext(event.path->path()) : "FOLDER" ) JSON_COMMA;
                        JSON_FIELD_STR("name", event.path->name());
                    JSON_OBJ_END;
                JSON_OBJ_END JSON_COMMA;
            }

            JSON_FIELD_STR("state", to_str(params.state)) JSON_COMMA;
            if (event.command_id.has_value()) {
                JSON_FIELD_INT("command_id", *event.command_id) JSON_COMMA;
            }
            if (state.transfer_id.has_value()) {
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
        int formatted = snprintf(path_buf, sizeof(path_buf), "%s/%s", base_path, child_name);
        // Name didn't fit. That, in theory, should not happen, but better safe than sorry...
        if (formatted >= FILE_NAME_BUFFER_LEN) {
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
} // namespace

tuple<JsonResult, size_t> PreviewRenderer::render(uint8_t *buffer, size_t buffer_size) {
    // base64 encodes 3 bytes to 4 ASCII chars, decoding needs to happen in multiples of this to work
    constexpr static size_t encoded_chunk_size = 4;
    constexpr static size_t decoded_chunk_size = 3;

    constexpr static const char *intro = "\"preview\":\"";
    constexpr static const char *outro = "\",";
    constexpr static size_t intro_len = strlen(intro);
    // Ending quote and comma
    constexpr static size_t outro_len = strlen(outro);
    // Don't bother with too small buffers to make the code easier. Extra char
    // for trying out there's some preview in there.
    constexpr static size_t min_len = intro_len + outro_len + encoded_chunk_size;

    if (buffer_size < min_len) {
        // Will be retried next time with bigger buffer.
        return make_tuple(JsonResult::BufferTooSmall, 0);
    }

    size_t written = 0;

    if (!started) {
        // get any thumbnail bigger than 17x17
        if (!gcode->get()->stream_thumbnail_start(17, 17, IGcodeReader::ImgType::PNG, true)) {
            // no thumbnail found in gcode, just dont send anything
            return make_tuple(JsonResult::Complete, 0);
        }
        // write intro
        memcpy(buffer, intro, intro_len);
        written += intro_len;
        buffer += intro_len;
        started = true;
    }

    bool write_end = false;
    while ((buffer_size - written) >= (encoded_chunk_size + 1)) { // if there is space for another chunk (and ending \0)
        // read chunk of decoded data
        uint8_t dec_chunk[decoded_chunk_size] = { 0 };
        size_t decoded_len = 0;
        while (decoded_len < decoded_chunk_size) {
            if (gcode->get()->stream_getc(reinterpret_cast<char &>(dec_chunk[decoded_len])) != IGcodeReader::Result_t::RESULT_OK) {
                // probably end of data, or error. Either way stop reading and send whatever was read till now.
                // if error happens while sending thumbnail, there is not much that can be done to signal that anyway.
                write_end = true;
                break;
            }
            ++decoded_len;
        }
        // encode data, if there is something to encode
        if (decoded_len == 0) {
            // nothing to encode, end
            break;
        }
        [[maybe_unused]] size_t encoded_len;
        // note that mbedtls_base64_encode also writes ending zero, but we want to skip that
        [[maybe_unused]] auto res = mbedtls_base64_encode(buffer, encoded_chunk_size + 1, &encoded_len, dec_chunk, decoded_len);
        assert(res == 0 && encoded_len == encoded_chunk_size); // should not fail, buffer should always be big enough
        written += encoded_chunk_size;
        buffer += encoded_chunk_size;
    }

    if (write_end && (buffer_size - written) >= outro_len) {
        // This is the end!
        memcpy(buffer, outro, outro_len);
        written += outro_len;
        return make_tuple(JsonResult::Complete, written);
    }

    return make_tuple(JsonResult::Incomplete, written);
}

tuple<JsonResult, size_t> GcodeMetaRenderer::render(uint8_t *buffer, size_t buffer_size) {
    assert(gcode->is_open());
    if (first_run) {
        gcode_line_buffer = GcodeBuffer(); // reset buffer
        if (!gcode->get()->stream_metadata_start()) {
            return make_tuple(JsonResult::Complete, 0);
        }
        first_run = false;
    }

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

    // This code iterates though metadata, and only if entire  key:value, fits to output buffer, send it.
    // If just part of the string fits, skip it and try to place it into next packed.
    while (true) {
        if (gcode_line_buffer.line.is_empty()) {
            // line is empty, that indicates that last line was already processed and we need to fetch another one
            if (gcode->get()->stream_get_line(gcode_line_buffer) != IGcodeReader::Result_t::RESULT_OK) {
                break;
            }
        }

        GcodeBuffer::String::parsed_metadata_t parsed = gcode_line_buffer.line.parse_metadata();
        if (parsed.first.begin == nullptr || parsed.second.begin == nullptr) {
            gcode_line_buffer = GcodeBuffer(); // reset buffer to fetch another line
            continue;
        }

        // Either result of putting something to the buffer, or nullopt if this line should be skipped.
        std::optional<JsonResult> result = nullopt;

        const auto filter = meta_filter(parsed.first.c_str());
        switch (filter) {
        case MetaFilter::Ignore:
            // do nothing, just go o next line
            break;
        case MetaFilter::String:
            result = output.output_field_str(0, parsed.first.c_str(), parsed.second.c_str());
            break;

        case MetaFilter::Float: {
            char *end = nullptr;
            double v = strtod(parsed.second.c_str(), &end);
            if (end != nullptr && *end != '\0') {
                // unable to parse, skip this
            } else {
                result = output.output_field_float_fixed(0, parsed.first.c_str(), v, 2);
            }
            break;
        }

        case MetaFilter::Int:
        case MetaFilter::Bool: {
            char *end = nullptr;
            long v = strtol(parsed.second.c_str(), &end, 10);
            if (end != nullptr && *end != '\0') {
                // Not really an int there. Skip this line.
            } else {
                if (filter == MetaFilter::Int) {
                    result = output.output_field_int(0, parsed.first.c_str(), v);
                } else {
                    // The gcode encodes bools as 0/1, JSON has True and False.
                    result = output.output_field_bool(0, parsed.first.c_str(), v);
                }
            }
            break;
        }
        }

        if (!result.has_value()) {
            // no result obtained from this line -> skip it
            gcode_line_buffer = GcodeBuffer();
            continue;
        }

        if (result.value() == JsonResult::Complete) {
            // Line successfully put to buffer - now put ending ","
            result = output.output(0, ",");
        }

        switch (result.value()) {
        case JsonResult::Complete:
            // Successfully put content into into the buffer. update pos, and reset buffer to go to next line
            pos = buffer_size - buffer_size_rest;
            gcode_line_buffer = GcodeBuffer();
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
        if (const char *lfn = dirent_lfn(state.ent); lfn && lfn[0] == '.') {
            // Skip dot-files (should be hidden).
            continue;
        }

        state.childsize = nullopt;
        // Will skip all the .bbf and other files still being transfered
        // (that's actually what we want, they are not usable until renamed).
        if (state.ent->d_type == DT_DIR && filename_is_printable(state.ent->d_name)) {
            MutablePath path(state.base_path);
            path.push(state.ent->d_name);
            // This also checks validity of the file
            if (auto st_opt = transfers::Transfer::get_transfer_partial_file_stat(path); st_opt.has_value()) {
                state.ent->d_type = DT_REG;
                state.childsize = st_opt->st_size;
                state.read_only = true;
            } else {
                continue;
            }
        } else {
            state.read_only = false;
        }
        state.child_cnt ++;

        if (!state.first) {
            JSON_COMMA;
        } else {
            state.first = false;
        }

        JSON_OBJ_START;
            JSON_FIELD_STR_437("name", state.ent->d_name) JSON_COMMA;
            JSON_FIELD_STR("display_name", dirent_lfn(state.ent)) JSON_COMMA;
            JSON_FIELD_INT("size", state.childsize.has_value() ? state.childsize.value() : child_size(state.base_path, state.ent->d_name)) JSON_COMMA;
#ifdef UNITTESTS
            // While "our" dirent contains time, the "real" one doesn't, so disable for unit tests
            JSON_FIELD_INT("m_timestamp", 0) JSON_COMMA;
#else
            JSON_FIELD_INT("m_timestamp", state.ent->time) JSON_COMMA;
#endif
            JSON_FIELD_BOOL("read_only", state.read_only) JSON_COMMA;
            JSON_FIELD_STR("type", file_type(state.ent));
        JSON_OBJ_END;
    }
    JSON_ARR_END JSON_COMMA;
    JSON_FIELD_INT("file_count", state.child_cnt) JSON_COMMA;
    JSON_END;
    // clang-format on
}

FileExtra::FileExtra(std::unique_ptr<AnyGcodeFormatReader> gcode_reader_)
    : gcode_reader(std::move(gcode_reader_))
    , renderer(std::move(GcodeExtra(PreviewRenderer(gcode_reader.get()), GcodeMetaRenderer(gcode_reader.get())))) {}

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
                path = params.job_path();
            }
            break;
        case EventType::FileInfo: {
            assert(event->path.has_value());
            SharedPath spath = event->path.value();
            path = spath.path();

            if (auto reader = std::make_unique<AnyGcodeFormatReader>(path); reader->is_open()) {
                // AnyGcodeFormatReader also handles partial files - so if this is actualy directory with partial file, it will be handled here
                file_extra = FileExtra(std::move(reader));
            } else if (unique_dir_ptr d(opendir(path)); d.get() != nullptr) {
                file_extra = FileExtra(path, std::move(d));
            } else if (unique_file_ptr f(fopen(path, "r")); f != nullptr) {
                // Non-gcode but existing file
                file_extra = FileExtra();
            } else {
                error = true;
            }
            // We are being rude here a bit. While the event is const, we modify the shared buffer. Nevertheless:
            // * The shared buffer is not shared into other threads, so nobody
            //   is reading it at the same time as we are writing into it.
            // * If this is ever called multiple times (it can be, if the same
            //   event needs to be resent), it results into the same values
            //   there.
            get_SFN_path(spath.path());
            get_LFN(spath.name(), FILE_NAME_BUFFER_LEN, spath.path());
            break;
        }
        case EventType::FileChanged: {
            assert(event->path.has_value());
            SharedPath spath = event->path.value();
            path = spath.path();

            get_SFN_path(spath.path());
            get_LFN(spath.name(), FILE_NAME_BUFFER_LEN, spath.path());
        }
        default:;
        }

        if (!error && path) {
            MutablePath mut_path(path);
            // Note: We allow only printable partial files in output and hide
            // all the rest. Other files are not usable until fully downloaded
            // & put into place (eg. a bbf), so we make sure Connect doesn't
            // get ideas about trying to use them.
            if (auto st_opt = transfers::Transfer::get_transfer_partial_file_stat(mut_path); st_opt.has_value() && filename_is_printable(path)) {
                has_stat = true;
                st = st_opt.value();
                read_only = true;
            } else if (stat(path, &st) == 0) {
                has_stat = true;
            }
        }

        // Some events override their transfer_id from another source, so we
        // replace it here to simplify the actual rendering. These events are
        // "after the fact" reports about the transfer, because they are
        // generated at the time when the transfer is _no longer running_.
        if (event->transfer_id.has_value()) {
            transfer_id = event->transfer_id;
        }
    }
}

JsonResult Renderer::renderState(size_t resume_point, JsonOutput &output, RenderState &state) const {
    return visit([&](auto action) -> JsonResult {
        return render_msg(resume_point, output, state, action);
    },
        state.action);
}

} // namespace connect_client
