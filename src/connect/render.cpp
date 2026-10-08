#include "render.hpp"
#include "printer_type.hpp"

#include <client_response.hpp>
#include <segmented_json_macros.h>
#include <lfn.h>
#include <filename_type.hpp>
#include <buddy/filename_defs.hpp>
#include <filepath_operation.h>
#include <timing.h>
#include <state/printer_state.hpp>
#include <transfers/transfer.hpp>
#include <filament.hpp>
#include <filament_list.hpp>
#include <feature/filament_sensor/filament_sensor_states.hpp>

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
    #include <feature/cancel_object/cancel_object.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <feature/chamber_filtration/chamber_filtration.hpp>
#endif

#include <bit>
#include <cstring>
#include <cinttypes>

#include <marlin_server_shared.h>
#include <mbedtls/base64.h>

#include <option/has_mmu2.h>
#include <option/has_filament_tracker.h>
#include <bsod/bsod.h>

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

    const char *to_str(Printer::FinishedJobResult result) {
        switch (result) {
        case Printer::FinishedJobResult::FIN_OK:
            return "FIN_OK";
        case Printer::FinishedJobResult::FIN_STOPPED:
            return "FIN_STOPPED";
        }
        return nullptr;
    }

#if PRINTER_IS_PRUSA_iX()
    const char *to_str(FilamentSensorState state) {
        switch (state) {
        case FilamentSensorState::NotInitialized:
            return "NOT_INITIALIZED";
        case FilamentSensorState::NotCalibrated:
            return "NOT_CALIBRATED";
        case FilamentSensorState::HasFilament:
            return "HAS_FILAMENT";
        case FilamentSensorState::NoFilament:
            return "NO_FILAMENT";
        case FilamentSensorState::NotConnected:
            return "NOT_CONNECTED";
        case FilamentSensorState::Disabled:
            return "DISABLED";
        }

        return "Unknown";
    }
#endif

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

    JsonResult render_msg(size_t resume_point, JsonOutput &output, RenderState &, const transfers::Download::InlineRequest &request) {
        // Keep the indentation of the JSON in here!
        // clang-format off
        JSON_START;
        JSON_OBJ_START;
            JSON_FIELD_STR("transfer", "inline") JSON_COMMA;
            if (request.details.has_value()) {
                JSON_FIELD_STR("hash", request.details->hash) JSON_COMMA;
                JSON_FIELD_INT("team_id", request.details->team_id) JSON_COMMA;
                JSON_FIELD_INT("transfer_id", request.details->transfer_id.to_uint32_t()) JSON_COMMA;
            }
            // Relates both to size of the FS block.
            JSON_FIELD_INT("chunk", 4096) JSON_COMMA;
            JSON_FIELD_INT("file_id", request.file_id) JSON_COMMA;
            JSON_FIELD_INT("start", request.start) JSON_COMMA;
            JSON_FIELD_INT("end", request.end);
        JSON_OBJ_END;
        JSON_END;
        // clang-format on
    }

    JsonResult render_msg(size_t resume_point, JsonOutput &output, RenderState &state, const SendTelemetry &telemetry) {
        const auto params = state.printer.params();

        const optional<Monitor::Status> transfer_status = get_transfer_status(resume_point, state);

        const auto iter_physical_tool = [&] { return *state.iter_physical_tool(); };
        const auto iter_virtual_tool = [&] { return *state.iter_virtual_tool(); };
        const auto iter_physical_tool_info = [&]() -> const Printer::PhysicalToolInfo & { return params.physical_tools[iter_physical_tool()]; };
        [[maybe_unused]] const auto iter_virtual_tool_info = [&]() -> const Printer::VirtualToolInfo & { return params.virtual_tools[iter_virtual_tool()]; };

#if PRINTER_IS_PRUSA_iX()
        const auto &preferred = params.physical_tools[state.preferred_physical_tool()];
        auto extruder_fs_state = preferred.extruder_fs_state;
        auto remote_fs_state = preferred.remote_fs_state;
#endif

        // Keep the indentation of the JSON in here!
        // clang-format off
        JSON_START;
        JSON_OBJ_START;
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
                JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_id", transfer_status->id.to_uint32_t()) JSON_COMMA;
                JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_transferred", transfer_status->download_progress.get_valid_size()) JSON_COMMA;
                JSON_FIELD_INT_G(transfer_status.has_value(), "transfer_time_remaining", transfer_status->time_remaining_estimate()) JSON_COMMA;
                JSON_FIELD_FFIXED_G(transfer_status.has_value(), "transfer_progress", transfer_status->progress_estimate() * 100.f, 1) JSON_COMMA;
            }

            // These are not included in the fingerprint as they are changing a lot.
            if (params.has_job) {
                JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
                JSON_FIELD_INT("time_printing", params.print_duration) JSON_COMMA;
                if (params.time_to_end != marlin_server::TIME_TO_END_INVALID) {
                    JSON_FIELD_INT("time_remaining", params.time_to_end) JSON_COMMA;
                }
                if (params.time_to_pause != marlin_server::TIME_TO_END_INVALID) {
                    // Connect calls it "filament change". Slicer "Time to
                    // color change". But in reality it is both pause and
                    // filament change (M600 / M601).
                    JSON_FIELD_INT("filament_change_in", params.time_to_pause) JSON_COMMA;
                }
                JSON_FIELD_INT("progress", params.progress_percent) JSON_COMMA;
            }

            if (telemetry.mode == SendTelemetry::Mode::Full) {
                JSON_FIELD_FFIXED("temp_bed", params.temp_bed, 1) JSON_COMMA;
#if PRINTER_IS_PRUSA_iX()
                JSON_FIELD_FFIXED("temp_heatbreak", preferred.temp_heatbreak, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("temp_psu", params.temp_psu, 1) JSON_COMMA;
                JSON_FIELD_FFIXED("temp_ambient", params.temp_ambient, 1) JSON_COMMA;
                if (extruder_fs_state) {
                    JSON_FIELD_STR_G(extruder_fs_state, "extruder_fs_state", to_str(*extruder_fs_state)) JSON_COMMA;
                }
                if (remote_fs_state) {
                    JSON_FIELD_STR_G(remote_fs_state, "remote_fs_state", to_str(*remote_fs_state)) JSON_COMMA;
                }
#endif
                JSON_FIELD_FFIXED("target_bed", params.target_bed, 1) JSON_COMMA;
                JSON_FIELD_INT("speed", params.print_speed) JSON_COMMA;
                JSON_FIELD_INT("flow", params.flow_factor) JSON_COMMA;
#if XL_ENCLOSURE_SUPPORT()
                if (params.enclosure_info.present) {
                    JSON_FIELD_OBJ("enclosure");
                        JSON_FIELD_INT("temp", params.enclosure_info.temp) JSON_COMMA;
                        JSON_FIELD_INT("fan_rpm", params.enclosure_info.fan_rpm) JSON_COMMA;
                        JSON_FIELD_INT("time_in_use", params.enclosure_info.time_in_use);
                    JSON_OBJ_END JSON_COMMA;
                }
#endif
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
                JSON_FIELD_OBJ("chamber");
                    JSON_FIELD_FFIXED("temp", params.chamber_info.current_temp, 1) JSON_COMMA;
                    JSON_FIELD_INT("target_temp", params.chamber_info.target_temp) JSON_COMMA;
                    JSON_FIELD_INT("fan_1_rpm", params.chamber_info.fan_1_rpm) JSON_COMMA;
                    JSON_FIELD_INT("fan_2_rpm", params.chamber_info.fan_2_rpm) JSON_COMMA;
                    JSON_FIELD_INT("fan_pwm_target", params.chamber_info.fan_pwm_target) JSON_COMMA;
                    JSON_FIELD_INT("led_intensity", params.chamber_info.led_intensity);
                JSON_OBJ_END JSON_COMMA;
#endif
                if (!params.has_job) {
                    // To avoid spamming the DB, connect doesn't want positions during printing
                    JSON_FIELD_FFIXED("axis_x", params.pos[Printer::X_AXIS_POS], 2) JSON_COMMA;
                    JSON_FIELD_FFIXED("axis_y", params.pos[Printer::Y_AXIS_POS], 2) JSON_COMMA;
                }
                JSON_FIELD_FFIXED("axis_z", params.pos[Printer::Z_AXIS_POS], 2) JSON_COMMA;

                JSON_FIELD_OBJ("tools");
                    for (state.iter = 0; state.iter < state.iter_tool_count(); state.iter++) {
                        if (state.iter > 0) {
                            JSON_COMMA;
                        }
                        if (state.iter_physical_tool().has_value()) {
                            if (!state.is_enabled(iter_physical_tool())) {
                                JSON_FIELD_NULL(to_connect_tool(iter_physical_tool()).data());
                                continue;
                            }
                            JSON_CUSTOM("\"%s\":{", to_connect_tool(iter_physical_tool()).data());
                                JSON_FIELD_STR("type", "FFF_PRINT_HEAD") JSON_COMMA;
                                JSON_FIELD_BOOL("active", state.is_active(iter_physical_tool())) JSON_COMMA;
                                JSON_FIELD_FFIXED("nozzle_temp", iter_physical_tool_info().temp_nozzle, 1) JSON_COMMA;
                                if (iter_physical_tool_info().target_nozzle != 0) {
                                    JSON_FIELD_INT("nozzle_target_temp", iter_physical_tool_info().target_nozzle) JSON_COMMA;
                                } else {
                                    JSON_FIELD_NULL("nozzle_target_temp") JSON_COMMA;
                                }
                                JSON_FIELD_INT("heatbreak_fan_rpm", iter_physical_tool_info().heatbreak_fan_rpm) JSON_COMMA;
                                JSON_FIELD_INT("print_fan_rpm", iter_physical_tool_info().print_fan_rpm) // No comma
                                // The feeder mirrors the one in INFO. A head with a feeder leaves consumed_weight to its slots.
#if HAS_MMU2()
                                if (state.tool_args.mmu_enabled) {
                                    JSON_COMMA; JSON_FIELD_OBJ("feeder");
                                        JSON_FIELD_INT("mmu_progress_code", params.progress_code);
                                        // The MMU reports 0 when idle, which has no character to send
                                        if (params.command_code != 0) {
                                            JSON_COMMA;
                                            JSON_FIELD_STR_FORMAT("mmu_command_in_progress", "%c", params.command_code);
                                        }
                                    JSON_OBJ_END;
                                }
#endif
                        }
                        else {
                            if (!state.is_enabled(iter_virtual_tool())) {
                                JSON_FIELD_NULL(to_connect_tool(iter_virtual_tool(), state.tool_args).data());
                                continue;
                            }

                            JSON_CUSTOM("\"%s\":{", to_connect_tool(iter_virtual_tool(), state.tool_args).data());
                            JSON_FIELD_BOOL("active", state.is_active(iter_virtual_tool()));

                            #if HAS_MMU2()
                                JSON_COMMA; JSON_FIELD_STR("type", "FFF_FEEDER_SLOT");
                            #else
                                bsod_unreachable();
                            #endif
                        }

                        if(state.iter_virtual_tool().has_value()) {
                            debug_assert(state.is_enabled(iter_virtual_tool()));

                            JSON_COMMA;
                            #if HAS_ANFC()
                                if(iter_virtual_tool_info().consumed_weight_g.has_value()) {
                                    JSON_FIELD_INT("consumed_weight", iter_virtual_tool_info().consumed_weight_g.value_or(0));
                                } else
                            #endif
                            {
                                JSON_FIELD_NULL("consumed_weight");
                            }

                            #if HAS_FILAMENT_TRACKER()
                                JSON_COMMA;
                                JSON_FIELD_INT("odometer_id", iter_virtual_tool_info().odometer_id) JSON_COMMA;
                                JSON_FIELD_INT("odometer_e", iter_virtual_tool_info().odometer_e_mm);
                            #endif
                        }
                        JSON_OBJ_END;
                    }
                JSON_OBJ_END JSON_COMMA;
            }
            if (state.background_command_id.has_value()) {
                JSON_FIELD_INT("command_id", *state.background_command_id) JSON_COMMA;
            }

            if (params.state.dialog.has_value()) {
                JSON_FIELD_INT_G(params.state.dialog.has_value(), "dialog_id", params.state.dialog->dialog_id.to_uint32_t()) JSON_COMMA;
            }
            // State is sent always, first because it seems important, but
            // also, we want something that doesn't have the final comma on
            // it.
            JSON_FIELD_STR("state", to_str(params.state.device_state));
        JSON_OBJ_END;
        JSON_END;
        // clang-format on
    }

    JsonResult render_msg(size_t resume_point, JsonOutput &output, RenderState &state, const Event &event) {
        const auto params = state.printer.params();
        const auto &info = state.printer.printer_info();

        const auto iter_physical_tool = [&] { return *state.iter_physical_tool(); };
        const auto iter_virtual_tool = [&] { return *state.iter_virtual_tool(); };
        const auto iter_physical_tool_info = [&]() -> const Printer::PhysicalToolInfo & { return params.physical_tools[iter_physical_tool()]; };
#if HAS_ANFC()
        std::array<char, UUID::string_length> uuid_buffer;
#endif

        const bool has_extra = (event.type != EventType::Accepted) && (event.type != EventType::Rejected);
        std::optional<Printer::FinishedJobResult> job_state;

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

        if (event.type == EventType::JobInfo && (!params.has_job || event.job_id.value_or(params.job_id) != params.job_id)) {
            // We have a job history (with just the state) of two last jobs, if this is one of them, send it,
            // otherwise reject.
            if (event.job_id.has_value()) {
                job_state = state.printer.get_prior_job_result(event.job_id.value());
            }
            if (job_state == nullopt) {
                reject_with = params.has_job ? "Job ID doesn't match" : "No job in progress";
            }
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
            if (has_extra && params.has_job) {
                JSON_FIELD_INT("job_id", params.job_id) JSON_COMMA;
            }

            if (event.reason != nullptr) {
                JSON_FIELD_STR("reason", event.reason) JSON_COMMA;
            }

            if (event.machine_reason != MachineReason::None) {
                JSON_FIELD_STR("machine_reason", to_str(event.machine_reason)) JSON_COMMA;
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
                    // TODO: Deprecated, kept for now for backwards compatibility. Remove eventually.
                    JSON_FIELD_BOOL("appendix", false) JSON_COMMA;
                    JSON_FIELD_STR("fingerprint", info.fingerprint) JSON_COMMA;
                    JSON_FIELD_BOOL("transfer_paused", !params.can_start_download) JSON_COMMA;
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
                            JSON_IP("lan_ipv4", state.lan->ip) JSON_COMMA;
                        }
#if HAS_ESP()
                        if (state.wifi.has_value()) {
                            if (strlen(creds.ssid) > 0) {
                                JSON_FIELD_STR("wifi_ssid", creds.ssid) JSON_COMMA;
                            }
                            JSON_MAC("wifi_mac", state.wifi->mac) JSON_COMMA;
                            JSON_IP("wifi_ipv4", state.wifi->ip) JSON_COMMA;
                        }
#endif
                        JSON_FIELD_STR("hostname", creds.hostname);
                    JSON_OBJ_END JSON_COMMA;

#if HAS_MMU2()
                    // The feeder block and the slots below belong to the only print head. Revisit once an MMU can sit
                    // on one of several heads (an XL with MMUs, or a plain head next to one with an MMU).
                    static_assert(PhysicalToolIndex::count == 1);
#endif
                    JSON_FIELD_OBJ("tools");
                        for (state.iter = 0; state.iter < state.iter_tool_count(); state.iter++) {
                            if (state.iter > 0) {
                                JSON_COMMA;
                            }
                            // Runs once per tool: a resumed chunk jumps past it
                            if (const auto virtual_tool = state.iter_virtual_tool()) {
                                state.material = state.printer.material(*virtual_tool);
                                state.virtual_tool_info = params.virtual_tools[*virtual_tool];
                            }
                            if (state.iter_physical_tool().has_value()) {
                                // Connect merges INFO into what it already has: null removes a tool, absence would keep it
                                if (!state.is_enabled(iter_physical_tool())) {
                                    JSON_FIELD_NULL(to_connect_tool(iter_physical_tool()).data());
                                    continue;
                                }
                                JSON_CUSTOM("\"%s\":{", to_connect_tool(iter_physical_tool()).data());
                                    JSON_FIELD_STR("type", "FFF_PRINT_HEAD") JSON_COMMA;
                                    JSON_FIELD_OBJ("features");
                                        JSON_FIELD_FFIXED("nozzle_diameter", iter_physical_tool_info().nozzle_diameter, 2) JSON_COMMA;
                                        JSON_FIELD_BOOL("nozzle_hardened", iter_physical_tool_info().hardened) JSON_COMMA;
                                        JSON_FIELD_BOOL("nozzle_high_flow", iter_physical_tool_info().high_flow);
                                    JSON_OBJ_END;
#if HAS_MMU2()
                                    JSON_COMMA;
                                    if (state.tool_args.mmu_enabled) {
                                        JSON_FIELD_OBJ("feeder");
                                            JSON_FIELD_STR("type", "MMU") JSON_COMMA;
                                            JSON_FIELD_STR("model", "MMU3") JSON_COMMA;
                                            JSON_FIELD_STR("base_model", "MMU3") JSON_COMMA;
                                            JSON_FIELD_INT("slot_count", VirtualToolIndex::count);
                                        JSON_OBJ_END;
                                    } else {
                                        // Removes a feeder reported before. Its slots stay out - without a feeder,
                                        // Connect does not look at them.
                                        JSON_FIELD_NULL("feeder");
                                    }
#endif
                            } else {
                                if (!state.is_enabled(iter_virtual_tool())) {
                                    JSON_FIELD_NULL(to_connect_tool(iter_virtual_tool(), state.tool_args).data());
                                    continue;
                                }

                                JSON_CUSTOM("\"%s\":{", to_connect_tool(iter_virtual_tool(), state.tool_args).data());

                                #if HAS_MMU2()
                                    JSON_FIELD_STR("type", "FFF_FEEDER_SLOT"); // No comma - last field
                                #else
                                    bsod_unreachable();
                                #endif
                            }

                            if (state.iter_virtual_tool().has_value()) {
                                JSON_COMMA;
                                if (!state.material.has_value()) {
                                    JSON_FIELD_NULL("material_package_instance");
                                } else {
                                    JSON_FIELD_OBJ("material_package_instance");
                                        JSON_FIELD_STR("class", "FFF");
#if HAS_ANFC()
                                        if (state.material->instance_uuid.has_value()) {
                                            JSON_COMMA;
                                            JSON_FIELD_STR("uuid", state.material->instance_uuid->to_string(uuid_buffer));
                                        }
                                        if (state.virtual_tool_info.full_weight_g.has_value() && !state.virtual_tool_info.full_weight_is_nominal) {
                                            JSON_COMMA;
                                            JSON_FIELD_INT("actual_netto_full_weight", *state.virtual_tool_info.full_weight_g);
                                        }
#endif
                                        JSON_COMMA;
                                        JSON_FIELD_OBJ("package");
                                            JSON_FIELD_STR("class", "FFF");
#if HAS_ANFC()
                                            if (state.material->package_uuid.has_value()) {
                                                JSON_COMMA;
                                                JSON_FIELD_STR("uuid", state.material->package_uuid->to_string(uuid_buffer));
                                            }
                                            if (state.virtual_tool_info.full_weight_g.has_value() && state.virtual_tool_info.full_weight_is_nominal) {
                                                JSON_COMMA;
                                                JSON_FIELD_INT("nominal_netto_full_weight", *state.virtual_tool_info.full_weight_g);
                                            }
#endif
                                            JSON_COMMA;
                                            JSON_FIELD_OBJ("material");
                                                JSON_FIELD_STR("class", "FFF");
#if HAS_ANFC()
                                                if (state.material->material_uuid.has_value()) {
                                                    JSON_COMMA;
                                                    JSON_FIELD_STR("uuid", state.material->material_uuid->to_string(uuid_buffer));
                                                }
#endif
                                                if (!state.material->name.empty()) {
                                                    JSON_COMMA;
                                                    JSON_FIELD_STR("abbreviation", state.material->name.data());
                                                }
                                                if (state.material->color.has_value()) {
                                                    JSON_COMMA;
                                                    JSON_FIELD_OBJ("primary_color");
                                                        // Color keeps 0x00RRGGBB in its raw value
                                                        JSON_FIELD_STR_FORMAT("color_rgba", "#%06" PRIx32, state.material->color->raw & 0xffffff);
                                                    JSON_OBJ_END;
                                                }
                                                JSON_COMMA;
                                                JSON_FIELD_ARR("tags");
                                                    if (state.material->is_abrasive) {
                                                        JSON_CUSTOM("\"%s\"", "abrasive");
                                                    }
#if HAS_CHAMBER_FILTRATION_API()
                                                    if (state.material->requires_filtration) {
                                                        if (state.material->is_abrasive) {
                                                            JSON_COMMA;
                                                        }
                                                        JSON_CUSTOM("\"%s\"", "filtration_recommended");
                                                    }
#endif
                                                JSON_ARR_END;
                                                JSON_COMMA;
                                                JSON_FIELD_OBJ("properties");
                                                    // The filament has one printing temperature, the architecture a range
                                                    JSON_FIELD_INT("min_print_temperature", state.material->nozzle_temperature) JSON_COMMA;
                                                    JSON_FIELD_INT("max_print_temperature", state.material->nozzle_temperature) JSON_COMMA;
                                                    JSON_FIELD_INT("preheat_temperature", state.material->nozzle_preheat_temperature) JSON_COMMA;
#if HAS_FILAMENT_HEATBREAK_PARAM()
                                                    JSON_FIELD_INT("heatbreak_temperature", state.material->heatbreak_temperature) JSON_COMMA;
#endif
#if HAS_CHAMBER_API()
                                                    if (state.material->chamber_min_temperature.has_value()) {
                                                        JSON_FIELD_INT("min_chamber_temperature", *state.material->chamber_min_temperature) JSON_COMMA;
                                                    }
                                                    if (state.material->chamber_max_temperature.has_value()) {
                                                        JSON_FIELD_INT("max_chamber_temperature", *state.material->chamber_max_temperature) JSON_COMMA;
                                                    }
                                                    if (state.material->chamber_target_temperature.has_value()) {
                                                        JSON_FIELD_INT("chamber_temperature", *state.material->chamber_target_temperature) JSON_COMMA;
                                                    }
#endif
                                                    JSON_FIELD_INT("min_bed_temperature", state.material->heatbed_temperature) JSON_COMMA;
                                                    JSON_FIELD_INT("max_bed_temperature", state.material->heatbed_temperature);
                                                JSON_OBJ_END;
                                            JSON_OBJ_END;
                                        JSON_OBJ_END;
                                    JSON_OBJ_END;
                                }
                            }
                            JSON_OBJ_END;
                        }
                    JSON_OBJ_END JSON_COMMA;

#if XL_ENCLOSURE_SUPPORT()
                    if (params.enclosure_info.present) {
                        JSON_FIELD_OBJ("enclosure");
                            JSON_FIELD_BOOL("enabled", params.enclosure_info.enabled) JSON_COMMA;
                            JSON_FIELD_BOOL("printing_filtration", params.enclosure_info.printing_filtration) JSON_COMMA;
                            JSON_FIELD_BOOL("post_print", params.enclosure_info.post_print) JSON_COMMA;
                            JSON_FIELD_INT("post_print_filtration_time", params.enclosure_info.post_print_filtration_time) JSON_COMMA;
                            JSON_FIELD_INT("filter_lifetime", buddy::chamber_filtration().filter_lifetime_s()) JSON_COMMA;
                            JSON_FIELD_ARR("filtration_filaments");
                            for (state.iter = 0, state.need_comma = false; state.iter <all_filament_types.size(); state.iter++) {
                                if(!all_filament_types[state.iter].parameters().requires_filtration) {
                                    continue;
                                }
                                if (state.need_comma) {
                                    JSON_COMMA;
                                }
                                JSON_CUSTOM("\"%s\"",  all_filament_types[state.iter].parameters().name.data());
                                state.need_comma = true;
                            }
                            JSON_ARR_END;
                        JSON_OBJ_END JSON_COMMA;
                    }
#endif
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
                    JSON_FIELD_BOOL("addon_power", params.addon_power) JSON_COMMA;
#endif
                    JSON_FIELD_INT("tool_count", PhysicalToolIndex::count);
                JSON_OBJ_END JSON_COMMA;
            } else if (event.type == EventType::JobInfo) {
                JSON_FIELD_OBJ("data");
                    if (job_state != nullopt) {
                        JSON_FIELD_STR("state", to_str(job_state.value()));
                    } else {
                        if (params.state.device_state == DeviceState::Printing) {
                            JSON_FIELD_STR("state", "PRINTING") JSON_COMMA;
                        } else {
                            JSON_FIELD_STR("state", "PAUSED") JSON_COMMA;
                        }
                        // The JobInfo doesn't claim the buffer, so we get it to store the path.
                        debug_assert(params.job_path() != nullptr);
                        if (state.has_stat) {
                            JSON_FIELD_INT("size", state.st.st_size) JSON_COMMA;
                            JSON_FIELD_INT("m_timestamp", state.st.st_mtime) JSON_COMMA;
                        }
                        if (params.job_lfn() != nullptr) {
                            JSON_FIELD_STR("display_name", params.job_lfn());
                        } else {
                            JSON_FIELD_STR("display_name", basename_b(params.job_path()));
                        }
                        JSON_COMMA;
                        if (event.start_cmd_id.has_value()) {
                            JSON_FIELD_INT("start_cmd_id", *event.start_cmd_id) JSON_COMMA;
                        }
                        JSON_FIELD_STR("path", params.job_path());
                    }
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

                    // BEWARE:
                    // If you add another field below, make sure to also
                    // include it in the blacklist of meta headers (see MetaFilter::Ignore).
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
                    JSON_FIELD_INT_G(transfer_status.has_value(), "size", transfer_status->expected) JSON_COMMA;
                    JSON_FIELD_INT_G(transfer_status.has_value(), "transferred", transfer_status->download_progress.get_valid_size()) JSON_COMMA;
                    JSON_FIELD_FFIXED_G(transfer_status.has_value(), "progress", transfer_status->progress_estimate() * 100.f, 1) JSON_COMMA;
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
                    if (event.incident == transfers::ChangedPath::Incident::Created) {
                        JSON_FIELD_STR("new_path", event.path->path()) JSON_COMMA;
                    } else if (event.incident == transfers::ChangedPath::Incident::Deleted) {
                        JSON_FIELD_STR("old_path", event.path->path()) JSON_COMMA;
                    } else /*Combined*/ {
                        JSON_FIELD_STR("new_path", event.path->path()) JSON_COMMA;
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
            } else if (event.type == EventType::CancelableChanged) {
#if HAS_CANCEL_OBJECT()
                JSON_FIELD_OBJ("data");
                    JSON_FIELD_ARR("objects");
                        state.iter = 0;

                        // Note: Because we're reading out object_count and is_object_cancelled directly,
                        // we can end up with inconsistent data being rendered.
                        // But that is fine, if that happens, cancel_object.revision changes and new render will be issued later, so we will eventually end up being consistent

                        while (static_cast<buddy::CancelObject::ObjectID>(state.iter) < buddy::cancel_object().object_count()) {
                            if (state.iter != 0) {
                                JSON_COMMA;
                            }
                            JSON_OBJ_START;
                                // is_object_cancelled will work even if i is outside of bounds, so having object_count inconsistent is fine
                                JSON_FIELD_BOOL("canceled", buddy::cancel_object().is_object_cancelled(state.iter)) JSON_COMMA;
                                JSON_FIELD_INT("id", state.iter);
                            JSON_OBJ_END;
                            state.iter++;
                        }
                    JSON_ARR_END;
                JSON_OBJ_END JSON_COMMA;
#endif
            } else if (event.type == EventType::StateChanged) {
                JSON_FIELD_OBJ("data");
                    // Unfortunately, we don't have any field that would be
                    // guaranteed to be present, so we need to do this insanity
                    // just to avoid a trailing comman, which is forbidden in
                    // JSON :-(
                    state.need_comma = false;

                    if (params.state.has_code()) {
                        state.need_comma = true;
                        // The additional value() check is there for the event
                        // where the below doesn't fit, we get resumed and
                        // the code disappears in between - in that case we
                        // kind of send a wrong value, but we will generate a
                        // new one soon after.
                        //
                        // (We could use the _GUARD version, but that one seems
                        // too drastic for this case).
                        JSON_FIELD_STR_FORMAT("code", "%05" PRIu16, params.state.code_num());
                    }

                    if (params.state.title()) {
                        if (state.need_comma) {
                            JSON_COMMA;
                        }

                        state.need_comma = true;

                        // Similar trick as above for the suspend/resume-race.
                        JSON_FIELD_STR("title", params.state.title() ? : "");
                    }

                    if (params.state.text()) {
                        if (state.need_comma) {
                            JSON_COMMA;
                        }

                        state.need_comma = true;

                        JSON_FIELD_STR("text", params.state.text() ? : "");
                    }

                    // We store the buttons here to preserve them across the
                    // resume points. This is fine, as these are all static
                    // global variables, nothing allocated dynamically.
                    //
                    // We do so to:
                    // * Make the iteration code simpler (no need to worry about changes).
                    // * Make sure the result is consistent set of buttons that
                    //   make sense to at least some dialog.
                    if ((state.buttons = params.state.buttons()) != nullptr) {
                        if (state.need_comma) {
                            JSON_COMMA;
                        }

                        state.need_comma = true;

                        JSON_FIELD_ARR("buttons");
                        state.iter = 0;
                        while (state.iter < MAX_RESPONSES) {
                            if (state.buttons[state.iter] == Response::_none) {
                                // We've run out of buttons.
                                break;
                            }
                            if (state.iter > 0) {
                                JSON_COMMA;
                            }
                            JSON_CUSTOM("\"%s\"", to_str(state.buttons[state.iter]));
                            state.iter ++;
                        }
                        JSON_ARR_END;
                    }
                JSON_OBJ_END JSON_COMMA;
            }

            if (params.state.dialog.has_value()) {
                JSON_FIELD_INT_G(params.state.dialog.has_value(), "dialog_id", params.state.dialog->dialog_id.to_uint32_t()) JSON_COMMA;
            }
            JSON_FIELD_STR("state", to_str(params.state.device_state)) JSON_COMMA;
            if (event.command_id.has_value()) {
                JSON_FIELD_INT("command_id", *event.command_id) JSON_COMMA;
            }
            if (state.transfer_id.has_value()) {
                JSON_FIELD_INT("transfer_id", (*state.transfer_id).to_uint32_t()) JSON_COMMA;
            }
            JSON_FIELD_STR("event", to_str(event.type));
        JSON_OBJ_END;
        JSON_END;
        // clang-format on
    }

    JsonResult render_msg(size_t, JsonOutput &, const RenderState &, const Sleep &) {
        // Sleep is handled on upper layers, not through renderer.
        debug_assert(0);
        return JsonResult::Abort;
    }

    JsonResult render_msg(size_t, JsonOutput &, const RenderState &, const ReadCommand &) {
        // Not a message to send to server
        debug_assert(0);
        return JsonResult::Abort;
    }

    std::optional<off_t> child_size(const char *base_path, const char *child_name) {
        char path_buf[filename_defs::path_buffer_size];
        int formatted = snprintf(path_buf, sizeof(path_buf), "%s/%s", base_path, child_name);
        // Path didn't fit. That, in theory, should not happen, but better safe than sorry...
        if (formatted >= static_cast<int>(filename_defs::path_buffer_size)) {
            return {};
        }
        struct stat st = {};
        if (stat(path_buf, &st) == 0) {
            return st.st_size;
        } else {
            return {};
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
        { "filament cost", MetaFilter::Float },
        { "filament used [mm]", MetaFilter::Float },
        { "filament used [cm3]", MetaFilter::Float },
        { "filament used [mm3]", MetaFilter::Float },
        { "filament used [m]", MetaFilter::Float },
        { "bed_temperature", MetaFilter::Int },
        { "brim_width", MetaFilter::Int },
        { "layer_height", MetaFilter::Float },
        { "temperature", MetaFilter::Int },
        // Note: These two should actually be Bools. But it seems the server is
        // currently expecting 0/1, the gcode also contains 0/1, so we adhere
        // to the rest because of compatibility.
        { "ironing", MetaFilter::Int },
        { "support_material", MetaFilter::Int },
        { "max_layer_z", MetaFilter::Float },
        { "estimated_print_time", MetaFilter::Int },
        { "total filament used for wipe tower [g]", MetaFilter::Float },

        // Blacklist of names not to send.
        // These really aren't metadata headers in common gcode files as far as
        // we know. But these are some additional fields we include in the data
        // object of the FILE_INFO event, so we protect against a collision by
        // malicious or confused gcode file.
        { "preview", MetaFilter::Ignore },
        { "size", MetaFilter::Ignore },
        { "m_timestamp", MetaFilter::Ignore },
        { "read_only", MetaFilter::Ignore },
        { "display_name", MetaFilter::Ignore },
        { "type", MetaFilter::Ignore },
        { "path", MetaFilter::Ignore },
    };

    MetaFilter meta_filter(const char *name) {
        for (size_t i = 0; i < sizeof meta_records / sizeof *meta_records; i++) {
            if (strcmp(name, meta_records[i].name) == 0) {
                return meta_records[i].filter;
            }
        }

        return MetaFilter::String;
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
    if (!thumbnail_reader) {
        // get any thumbnail bigger than 17x17
        thumbnail_reader = gcode->stream_thumbnail_start(17, 17, IGcodeReader::ImgType::PNG, true);
        if (!thumbnail_reader) {
            // no thumbnail found in gcode, just dont send anything
            return make_tuple(JsonResult::Complete, 0);
        }
        // write intro
        memcpy(buffer, intro, intro_len);
        written += intro_len;
        buffer += intro_len;
    }

    bool write_end = false;
    while ((buffer_size - written) >= (encoded_chunk_size + 1)) { // if there is space for another chunk (and ending \0)
        // read chunk of decoded data
        uint8_t dec_chunk[decoded_chunk_size] = { 0 };
        size_t decoded_len = thumbnail_reader->read({ reinterpret_cast<std::byte *>(dec_chunk), decoded_chunk_size }).size();
        if (decoded_len != decoded_chunk_size) {
            // probably end of data, or error. Either way stop reading and send whatever was read till now.
            // if error happens while sending thumbnail, there is not much that can be done to signal that anyway.
            write_end = true;
            break;
        }
        [[maybe_unused]] size_t encoded_len;
        // note that mbedtls_base64_encode also writes ending zero, but we want to skip that
        [[maybe_unused]] auto res = mbedtls_base64_encode(buffer, encoded_chunk_size + 1, &encoded_len, dec_chunk, decoded_len);
        debug_assert(res == 0 && encoded_len == encoded_chunk_size); // should not fail, buffer should always be big enough
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

void GcodeMetaRenderer::reset_buffer() {
    gcode_line_buffer.line = GcodeBuffer::String();
    parsed.reset();
}

JsonResult GcodeMetaRenderer::out_str_chunk(JsonOutput &output, const GcodeBuffer::String &str) {
    auto result = output.output_str_chunk(0, std::string_view { str.begin, str.len() });

    if (result == JsonResult::Complete && gcode_line_buffer.line_complete) {
        result = output.output(0, "\"");
    }

    if (result == JsonResult::Complete) {
        // Adjust this only if we were successful - if not, we'll retry with the same stuff.
        str_continuation = !gcode_line_buffer.line_complete;
    }

    return result;
}

tuple<JsonResult, size_t> GcodeMetaRenderer::render(uint8_t *buffer, size_t buffer_size) {
    if (first_run) {
        reset_buffer();
        if (!gcode->stream_metadata_start()) {
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
            if (gcode->stream_get_line(gcode_line_buffer, IGcodeReader::Continuations::Split) != IGcodeReader::Result_t::RESULT_OK) {
                break;
            }
        }

        // Either result of putting something to the buffer, or nullopt if this line should be skipped.
        std::optional<JsonResult> result = nullopt;

        if (str_continuation) {
            // Will adjust str_continuation as needed
            result = out_str_chunk(output, gcode_line_buffer.line);
        } else {
            // Disallow terminating the value in case it's taking all the 81 chars
            // ‒ that could touch the 82th char and we don't have that one.
            // (possibility with Split continuation of reading).
            //
            // (It probably can happen only in case the line_complete == false, but
            // that would look like a fragile assumption, so basing it off the real
            // "problem").
            const bool full_size = gcode_line_buffer.line.len() == gcode_line_buffer.buffer.size();
            if (!parsed.has_value()) {
                parsed = gcode_line_buffer.line.parse_metadata(!full_size);
            }
            if (parsed->first.begin == nullptr || parsed->second.begin == nullptr) {
                reset_buffer(); // reset buffer to fetch another line
                continue;
            }

            auto filter = meta_filter(parsed->first.c_str());

            // Too large headers are only handled and allowed for strings, others
            // aren't expected to exceed 80 chars.
            if (filter != MetaFilter::String && (full_size || !gcode_line_buffer.line_complete)) {
                // Eat the rest of the header.
                bool error = false;
                while (!gcode_line_buffer.line_complete) {
                    if (gcode->stream_get_line(gcode_line_buffer, IGcodeReader::Continuations::Split) != IGcodeReader::Result_t::RESULT_OK) {
                        error = true;
                        break;
                    }
                }

                if (error) {
                    break;
                }

                filter = MetaFilter::Ignore;
            }

            switch (filter) {
            case MetaFilter::Ignore:
                // do nothing, just go o next line
                break;
            case MetaFilter::String:
                // Only the name of the field and starting "
                result = output.output(0, "\"%s\":\"", parsed->first.c_str());
                if (result == JsonResult::Complete) {
                    // Will adjust the str_continuation as needed.
                    result = out_str_chunk(output, parsed->second);
                }
                break;

            case MetaFilter::Float: {
                char *end = nullptr;
                double v = strtod(parsed->second.c_str(), &end);
                if (end != nullptr && *end != '\0') {
                    // unable to parse, skip this
                } else {
                    result = output.output_field_float_fixed(0, parsed->first.c_str(), v, 2);
                }
                break;
            }

            case MetaFilter::Int:
            case MetaFilter::Bool: {
                char *end = nullptr;
                long v = strtol(parsed->second.c_str(), &end, 10);
                if (end != nullptr && *end != '\0') {
                    // Not really an int there. Skip this line.
                } else {
                    if (filter == MetaFilter::Int) {
                        result = output.output_field_int(0, parsed->first.c_str(), v);
                    } else {
                        // The gcode encodes bools as 0/1, JSON has True and False.
                        result = output.output_field_bool(0, parsed->first.c_str(), v);
                    }
                }
                break;
            }
            }
        }

        if (!result.has_value()) {
            // no result obtained from this line -> skip it
            reset_buffer();
            continue;
        }

        if (result.value() == JsonResult::Complete && !str_continuation) {
            // Line successfully put to buffer - now put ending ","
            result = output.output(0, ",");
        }

        switch (result.value()) {
        case JsonResult::Complete:
            // Successfully put content into into the buffer. update pos, and reset buffer to go to next line
            pos = buffer_size - buffer_size_rest;
            reset_buffer();
            break;
        case JsonResult::Abort:
            // We use only the primitive output functions and they are not
            // capable of returning Abort.
            debug_assert(0);
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

DirRenderer::DirRenderer(const char *base_path, Directory dir)
    : JsonRenderer(DirState { move(dir), base_path }) {}

JsonResult DirRenderer::renderState(size_t resume_point, json::JsonOutput &output, DirState &state) const {
    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_FIELD_ARR("children");
    while ((state.ent = state.dir.read())) {
        if (const char *lfn = dirent_lfn(state.ent); lfn && lfn[0] == '.') {
            // Skip dot-files (should be hidden).
            continue;
        }

        state.childsize = nullopt;
        if (state.ent->d_type == DT_DIR && filename_is_transferrable(state.ent->d_name)) {
            // Suspicion: This might actualy be a partial file. Check and decide what to do about it.
            const bool is_printable = filename_is_printable(state.ent->d_name);
            MutablePath path(state.base_path);
            path.push(state.ent->d_name);
            auto st_opt = transfers::Transfer::get_transfer_partial_file_stat(path);
            if (st_opt.has_value() && is_printable) {
                // A print file ‒ report it even while we are downloading, it can be printed right away.
                state.ent->d_type = DT_REG;
                state.childsize = st_opt->st_size;
                state.read_only = true;
            } else if (st_opt.has_value()) {
                // This is a bbf that's being transfered. Hide it completely until it is complete.
                continue;
            } else {
                // It is a directory with a stupid name, but not a running
                // transfer. Act as if it is just a directory.
                state.read_only = false;
                state.childsize = child_size(state.base_path, state.ent->d_name);
            }
        } else {
            state.read_only = false;
            state.childsize = child_size(state.base_path, state.ent->d_name);
        }

        state.child_cnt ++;

        if (!state.first) {
            JSON_COMMA;
        } else {
            state.first = false;
        }

        JSON_OBJ_START;
            JSON_FIELD_STR("name", state.ent->d_name) JSON_COMMA;
            JSON_FIELD_STR("display_name", dirent_lfn(state.ent)) JSON_COMMA;
            JSON_FIELD_INT("size", state.childsize.value_or(0)) JSON_COMMA;
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
    , renderer(std::move(GcodeExtra(PreviewRenderer(gcode_reader->get()), GcodeMetaRenderer(gcode_reader->get())))) {}

FileExtra::FileExtra(const char *base_path, Directory dir)
    : renderer(move(DirRenderer(base_path, move(dir)))) {}

#if PRINTER_IS_PRUSA_iX()
PhysicalToolIndex RenderState::preferred_physical_tool() const {
    return match(
        active_physical_tool,
        [](PhysicalToolIndex pt) { return pt; },
        // Nothing picked - take the first enabled tool (or the very first one on the error printer, which has none)
        [this](NoTool) { return PhysicalToolIndex::from_raw(physical_tool_mask == 0 ? 0 : std::countr_zero(physical_tool_mask)); });
}
#endif

RenderState::RenderState(const Printer &printer, const Action &action, optional<CommandId> background_command_id)
    : printer(printer)
    , action(action)
    , lan(printer.net_info(Printer::Iface::Ethernet))
#if HAS_ESP()
    , wifi(printer.net_info(Printer::Iface::Wifi))
#endif
    , transfer_id(Monitor::instance.id())
    , background_command_id(background_command_id) {
    memset(&st, 0, sizeof st);

    const auto params = printer.params();
    physical_tool_mask = params.physical_tool_mask;
    virtual_tool_mask = params.virtual_tool_mask;
    active_physical_tool = params.active_physical_tool;
    active_virtual_tool_mask = params.active_virtual_tool_mask;
    tool_args = params.tool_args;

    if (const auto *event = get_if<Event>(&action); event != nullptr) {
        const char *path = nullptr;
        bool error = false;

        switch (event->type) {
        case EventType::JobInfo:
            if (params.has_job) {
                path = params.job_path();
            }
            break;
        case EventType::FileInfo: {
            debug_assert(event->path.has_value());
            SharedPath spath = event->path.value();
            path = spath.path();

            if (auto reader = std::make_unique<AnyGcodeFormatReader>(path); reader->is_open()) {
                // AnyGcodeFormatReader also handles partial files - so if this is actualy directory with partial file, it will be handled here
                if ((*reader)->fully_valid()) {
                    file_extra = FileExtra(std::move(reader));
                } else {
                    // Avoid trying to send metadata/previews from partially transfered files, because:
                    // * We are not sure this'll succeed, maybe we don't have
                    //   enough downloaded. In that case we would abort sending
                    //   the data, kill the connection and do other ugly things.
                    // * Sending the data is very expensive and competes for
                    //   resources with the download. By postponing the send of
                    //   the data _after_ it was fully downloaded (which we
                    //   trigger on our own), we try to avoid some of it.
                    file_extra = FileExtra();
                }
            } else if (Directory dir { path }; dir) {
                file_extra = FileExtra(path, std::move(dir));
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
            get_SFN_path(spath.path_span());
            get_LFN(spath.name(), filename_defs::filename_buffer_size, spath.path());
            break;
        }
        case EventType::FileChanged: {
            debug_assert(event->path.has_value());
            SharedPath spath = event->path.value();
            path = spath.path();

            get_SFN_path(spath.path_span());
            get_LFN(spath.name(), filename_defs::filename_buffer_size, spath.path());
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

size_t RenderState::iter_tool_count() const {
#if HAS_MMU2()
    // 1 Tool with a feeder + X feeder slots
    static_assert(PhysicalToolIndex::count == 1);
    if (tool_args.mmu_enabled) {
        return 1 + VirtualToolIndex::count;
    } else {
        static_assert(PhysicalToolIndex::count == 1);
        return 1;
    }
#else
    static_assert(PhysicalToolIndex::count == VirtualToolIndex::count);
#endif

    return VirtualToolIndex::count;
}

std::optional<PhysicalToolIndex> RenderState::iter_physical_tool() const {
#if HAS_MMU2()
    static_assert(PhysicalToolIndex::count == 1);
    if (tool_args.mmu_enabled) {
        if (iter == 0) {
            return PhysicalToolIndex::from_raw(0);
        } else {
            return std::nullopt;
        }
    }
#endif

    return VirtualToolIndex::from_raw(iter).to_physical();
}

std::optional<VirtualToolIndex> RenderState::iter_virtual_tool() const {
#if HAS_MMU2()
    static_assert(PhysicalToolIndex::count == 1);
    if (tool_args.mmu_enabled) {
        if (iter == 0) {
            return std::nullopt;
        } else {
            return VirtualToolIndex::from_raw(iter - 1);
        }
    }
#endif

    return VirtualToolIndex::from_raw(iter);
}
} // namespace connect_client
