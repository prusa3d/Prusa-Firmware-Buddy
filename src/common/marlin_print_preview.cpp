/**
 * @file marlin_print_preview.cpp
 */

#include "marlin_print_preview.hpp"
#include <M73_PE.h>
#include "client_fsm_types.h"
#include "client_response.hpp"
#include "general_response.hpp"
#include "marlin_server.hpp"
#include <media_prefetch_instance.hpp>
#include "timing.h"
#include "filament_sensors_handler.hpp"
#include "filament.hpp"
#include "M70X.hpp"
#include <option/developer_mode.h>
#include "printers.h"
#include <Marlin/src/module/motion.h>
#include <option/has_gui.h>
#if HAS_GUI()
    #include "screen_menu_filament_changeall.hpp"
    #include "box_unfinished_selftest.hpp"
#endif
#include <option/has_selftest.h>

#include <option/has_toolchanger.h>
#if ENABLED(PRUSA_TOOLCHANGER)
    #include <module/prusa/toolchanger.h>
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/

#include <config_store/store_instance.hpp>
#include "tools_mapping.hpp"
#include <module/prusa/tool_mapper.hpp>
#include <module/prusa/spool_join.hpp>
#include <mmu2_toolchanger_common.hpp>
#include <common/gcode/gcode_info_scan.hpp>

// would be nice to have option leave phase as it was
// something like std::pair<enum {delete, leave, has_value },PhasesPrintPreview>
// but not currently needed
std::optional<PhasesPrintPreview> IPrintPreview::getCorrespondingPhase(IPrintPreview::State state) {
    switch (state) {

    case State::inactive:
        return std::nullopt;

    case State::init:
    case State::loading:
        return PhasesPrintPreview::loading;

    case State::download_wait:
        return PhasesPrintPreview::download_wait;

    case State::preview_wait_user:
        return PhasesPrintPreview::main_dialog;

    case State::unfinished_selftest_wait_user:
        return PhasesPrintPreview::unfinished_selftest;

    case State::new_firmware_available_wait_user:
        return PhasesPrintPreview::new_firmware_available;

#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::tools_mapping_wait_user:
        return PhasesPrintPreview::tools_mapping;
#endif

    case State::wrong_printer_wait_user:
        return PhasesPrintPreview::wrong_printer;

    case State::wrong_printer_wait_user_abort:
        return PhasesPrintPreview::wrong_printer_abort;

    case State::filament_not_inserted_wait_user:
    case State::filament_not_inserted_load:
        return PhasesPrintPreview::filament_not_inserted;

#if HAS_MMU2()
    case State::mmu_filament_inserted_wait_user:
    case State::mmu_filament_inserted_unload:
        return PhasesPrintPreview::mmu_filament_inserted;
#endif
    case State::wrong_filament_wait_user:
    case State::wrong_filament_change:
        return PhasesPrintPreview::wrong_filament;

    case State::file_error_wait_user:
        return PhasesPrintPreview::file_error;

    case State::checks_done:
    case State::done:
        return std::nullopt;
    }
    return std::nullopt;
}

void IPrintPreview::ChangeState(State s) {
    state = s;
    if (s != State::checks_done && s != State::init) { // don't inform about this state, since it's an internal one meant to be as a skip-through to proper state
        setFsm(getCorrespondingPhase(state));
    }
}

void IPrintPreview::setFsm(std::optional<PhasesPrintPreview> wantedPhase) {
    FSM_action action = IsFSM_action_needed(phase, wantedPhase);
    switch (action) {

    case FSM_action::no_action:
        break;

    case FSM_action::create:
        marlin_server::fsm_create(wantedPhase.value_or(PhasesPrintPreview::loading));
        break;

    case FSM_action::destroy:
        // do not call marlin_server::fsm_destroy(ClientFSM::PrintPreview);
        // we need to call it manually later to be atomic
        break;

    case FSM_action::change:
        marlin_server::fsm_change(*wantedPhase); // wantedPhase is not nullopt, FSM_action would not be change otherwise
        break;
    }
    phase = wantedPhase;
}

Response IPrintPreview::GetResponse() {
    return phase ? marlin_server::get_response_from_phase(*phase) : Response::_none;
}

static bool is_same(const char *curr_filament, const GCodeInfo::filament_buff &filament_type) {
    return strncmp(curr_filament, filament_type.begin(), filament_type.size()) == 0;
}
static bool filament_known(const char *curr_filament) {
    return strncmp(curr_filament, "---", 3) != 0;
}

#if ENABLED(PRUSA_SPOOL_JOIN) && ENABLED(PRUSA_TOOL_MAPPING)

bool PrintPreview::ToolsMappingValidty::all_ok() const {
    return unassigned_gcodes.count() == 0 &&
    #if not HAS_MMU2()
        mismatched_filaments.count() == 0 &&
    #endif
        mismatched_nozzles.count() == 0 && unloaded_tools.count() == 0;
}

auto PrintPreview::check_tools_mapping_validity(const ToolMapper &mapper, const SpoolJoin &joiner, const GCodeInfo &gcode) -> ToolsMappingValidty {
    ToolsMappingValidty result;

    // unassigned gcode check
    for (int gcode_tool = 0; gcode_tool < gcode.GivenExtrudersCount(); ++gcode_tool) {
        if (!gcode.get_extruder_info(gcode_tool).used()) {
            continue;
        }

        if (mapper.to_physical(gcode_tool) == ToolMapper::NO_TOOL_MAPPED) {
            result.unassigned_gcodes.set(gcode_tool);
        }
    }

    auto get_nozzle_diameter = [&]([[maybe_unused]] size_t idx) {
    #if HAS_TOOLCHANGER()
        return config_store().get_nozzle_diameter(idx);
    #elif HAS_MMU2()
        return config_store().get_nozzle_diameter(0);
    #endif // else unknown configuration
    };

    auto nozzles_match = [&](uint8_t physical_extruder) {
        auto gcode_tool = tools_mapping::to_gcode_tool_custom(mapper, joiner, physical_extruder);
        if (gcode_tool == tools_mapping::no_tool
            || !gcode.get_extruder_info(gcode_tool).used()
            || !gcode.get_extruder_info(gcode_tool).nozzle_diameter.has_value()) {
            return true;
        }

        float nozzle_diameter_distance = std::abs(static_cast<float>(gcode.get_extruder_info(gcode_tool).nozzle_diameter.value()) - static_cast<float>(get_nozzle_diameter(physical_extruder)));
        if (nozzle_diameter_distance > 0.001f) {
            return false;
        }

        return true;
    };

    auto tool_needs_to_be_loaded = [&]([[maybe_unused]] uint8_t physical_extruder) { // if any tool needs filament load
    #if HAS_TOOLCHANGER()
        if (!config_store().fsensor_enabled.get()) {
            return false;
        }

        return PrintPreview::check_extruder_need_filament_load(physical_extruder, ToolMapper::NO_TOOL_MAPPED, [&](uint8_t pe) {
            return tools_mapping::to_gcode_tool_custom(mapper, joiner, pe);
        });
    #elif HAS_MMU2()
        return false; // MMU is purposefully unloaded before print
    #endif
    };

    auto tool_has_correct_filament_type = [&](uint8_t physical_extruder) {
        return PrintPreview::check_correct_filament_type(physical_extruder, ToolMapper::NO_TOOL_MAPPED, [&](uint8_t pe) {
            return tools_mapping::to_gcode_tool_custom(mapper, joiner, pe);
        });
    };

    // The other 3 checks
    for (size_t physical = 0; physical < EXTRUDERS; ++physical) {
        if (!is_tool_enabled(physical)) {
            continue;
        }
        if (tool_needs_to_be_loaded(physical)) {
            result.unloaded_tools.set(physical);
        }
        if (!nozzles_match(physical)) {
            result.mismatched_nozzles.set(physical);
        }
        if (!tool_has_correct_filament_type(physical)) {
            result.mismatched_filaments.set(physical);
        }
    }

    return result;
}

#endif

bool PrintPreview::check_extruder_need_filament_load(uint8_t physical_extruder, uint8_t no_gcode_value, stdext::inplace_function<uint8_t(uint8_t)> gcode_extruder_getter) {
    auto gcode_extruder = gcode_extruder_getter(physical_extruder);
    if (gcode_extruder == no_gcode_value) {
        return false; // if this physical_extruder is not printing, no need to check its filament
    }

    if (!GCodeInfo::getInstance().get_extruder_info(gcode_extruder).used()) {
        return false;
    }

    // when tool doesn't have filament, it needs load
    return !FSensors_instance().ToolHasFilament(physical_extruder);
}

static bool check_extruder_need_filament_load_tools_mapping(uint8_t physical_extruder) {
    return PrintPreview::check_extruder_need_filament_load(physical_extruder, tools_mapping::no_tool, tools_mapping::to_gcode_tool);
}

bool PrintPreview::check_correct_filament_type(uint8_t physical_extruder, uint8_t no_gcode_value, stdext::inplace_function<uint8_t(uint8_t)> gcode_extruder_getter) {
    const auto gcode_extruder = gcode_extruder_getter(physical_extruder);
    if (gcode_extruder == no_gcode_value) {
        return true; // nothing to check, this extruder doesn't print anything
    }

    const auto &extruder_info = GCodeInfo::getInstance().get_extruder_info(gcode_extruder);
    if (!extruder_info.used()) {
        return true; // when tool not used in print, return OK filament type
    }

    if (!extruder_info.filament_name.has_value()) {
        return true; // filament type unspecified, return tool OK
    }

    const FilamentType loaded_filament_type = config_store().get_filament_type(physical_extruder);
    const auto loaded_filament_name = filament::get_name(loaded_filament_type);
    // when loaded filament type not known, return that filament type is OK
    return !filament_known(extruder_info.filament_name.value().data()) || is_same(loaded_filament_name, extruder_info.filament_name.value());
}

static bool check_correct_filament_type_tools_mapping(uint8_t physical_extruder) {
    return PrintPreview::check_correct_filament_type(physical_extruder, tools_mapping::no_tool, tools_mapping::to_gcode_tool);
}

IPrintPreview::State PrintPreview::stateFromFilamentPresence() const {

#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        if (!config_store().fsensor_enabled.get()) {
            return State::checks_done;
        }
        // with MMU, its only possible to check that filament is properly unloaded, no check of filaments presence in each "tool"
        if (FSensors_instance().MMUReadyToPrint()) {
            return State::checks_done;
        } else if (GCodeInfo::getInstance().is_singletool_gcode() && FSensors_instance().WhereIsFilament() == MMU2::FilamentState::AT_FSENSOR) {
            return State::checks_done; // it makes sense to allow starting a single material print with filament loaded
        } else {
            return State::mmu_filament_inserted_wait_user;
        }
    } else
#endif
    {
        if (!config_store().fsensor_enabled.get()) {
            return stateFromFilamentType();
        }
        if (tools_mapping::is_tool_mapping_possible()) {
            return stateFromFilamentType(); // filament loaded/type checks are handled by the tools_mapping screen
        }

        // no MMU, do regular check of filament presence in each tool
        EXTRUDER_LOOP() { // e == physical_extruder
            if (check_extruder_need_filament_load_tools_mapping(e)) {
                return State::filament_not_inserted_wait_user;
            }
        }
        return stateFromFilamentType();
    }
}

static void queue_filament_load_gcodes() {
    // Queue load filament gcode for every tool that doesn't have filament loaded
    EXTRUDER_LOOP() { // e == physical_extruder
        auto gcode_extruder = tools_mapping::to_gcode_tool(e);
        if (gcode_extruder == tools_mapping::no_tool) {
            // if this physical extruder is not printing, no need to load anything for it
            continue;
        }

        // skip for tools that already have filament
        if (!check_extruder_need_filament_load_tools_mapping(e)) {
            continue;
        }

        // pass filament type from gcode, so that user doesn't have to select filament type
        const char *filament_name = GCodeInfo::getInstance().get_extruder_info(gcode_extruder).filament_name.has_value()
            ? GCodeInfo::getInstance().get_extruder_info(gcode_extruder).filament_name.value().data()
            : "";
#if HOTENDS > 1
        // if printer has multiple hotends (eg: XL), preheat all that will be loaded to save time for user
        auto target_temp = filament::get_description(filament::get_type(filament_name, strlen(filament_name))).nozzle_temperature;
        thermalManager.setTargetHotend(target_temp, e);
        marlin_server::set_temp_to_display(target_temp, e);
#endif
        marlin_server::enqueue_gcode_printf("M701 S\"%s\" T%d W2", filament_name, e);
    }
}

static void queue_filament_change_gcodes() {
    // Queue change filament gcode for every tool with mismatched filament type
    EXTRUDER_LOOP() { // e == physical_extruder
        auto gcode_extruder = tools_mapping::to_gcode_tool(e);
        if (gcode_extruder == tools_mapping::no_tool) {
            continue; // this extruder doesn't print anything
        }

        // already has corrent filament type
        if (check_correct_filament_type_tools_mapping(e)) {
            continue;
        }

        // pass filament type from gcode, so that user doesn't have to select filament type
        const char *filament_name = GCodeInfo::getInstance().get_extruder_info(gcode_extruder).filament_name.has_value()
            ? GCodeInfo::getInstance().get_extruder_info(gcode_extruder).filament_name.value().data()
            : "";

#if HOTENDS > 1 // Here we would love mapping of extruder -> hotend, but since we don't have it, this check will have to suffice
        // if printer has multiple hotends (eg: XL), preheat all that will be loaded to save time for user
        auto temp_old = filament::get_description(config_store().get_filament_type(e)).nozzle_temperature;

        thermalManager.setTargetHotend(temp_old, e);
        marlin_server::set_temp_to_display(temp_old, e);
#endif

        // M1600 - change, R - add return option, U1 - Ask filament type if unknown, T - tool, Sxxx - preselect filament type
        marlin_server::enqueue_gcode_printf("M1600 S\"%s\" T%d R U1", filament_name, e);
    }
}

IPrintPreview::State PrintPreview::stateFromFilamentType() const {
    if (tools_mapping::is_tool_mapping_possible()) {
        return State::checks_done; // filament loaded/type checks are handled by the tools_mapping screen
    }

    // Check match of loaded and G-code types
    EXTRUDER_LOOP() { // e == physical_extruder
        if (!check_correct_filament_type_tools_mapping(e)) {
            return State::wrong_filament_wait_user;
        }
    }
    return State::checks_done;
}

void PrintPreview::tools_mapping_cleanup(bool leaving_to_print) {
    if (!leaving_to_print) {
        // stop preheating bed
        marlin_server::set_target_bed(0);
#if ENABLED(PRUSA_TOOL_MAPPING)
        tool_mapper.reset();
        spool_join.reset();
#endif
    }

#if PRINTER_IS_PRUSA_XL
    // set dwarf leds to be handled 'normally'
    HOTEND_LOOP() {
        prusa_toolchanger.getTool(e).set_cheese_led(); // Default LED config
        prusa_toolchanger.getTool(e).set_status_led(); // Default status LED
    }
#endif
}

PrintPreview::Result PrintPreview::Loop() {
    if (GetState() == State::inactive) {
        return Result::Inactive;
    }

    const uint32_t curr_ms = ticks_ms();
    if (ticks_diff(curr_ms, last_run) < max_run_period_ms) {
        return stateToResult();
    }
    last_run = curr_ms;
    const Response response = GetResponse();

    auto &gcode_info = GCodeInfo::getInstance();

    switch (GetState()) {

    case State::inactive: // cannot be, but have it defined to enumerate all states
        return Result::Inactive;

    case State::init:
        gcode_info_scan::start_scan();

        // Reset print progress to 0. Need to be at this point because Connect is already starting to snitch the info.
        oProgressData.mInit();

        ChangeState(State::loading);
        if (skip_if_able > marlin_server::PreviewSkipIfAble::no) {
            // if skip print confirmation was requested, mark the print as started immediately.
            // If not, it will be started later when user clicks print
            return Result::MarkStarted;
        }
        break;

    case State::download_wait:
        if (response == Response::Quit) {
            gcode_info_scan::cancel_scan();
            ChangeState(State::inactive);
            return Result::Abort;

        } else if (gcode_info.has_error()) {
            ChangeState(State::file_error_wait_user);

        } else if (!gcode_info.can_be_printed()) {
            // Wait till we have downloaded enough

        } else if (!marlin_server::media_prefetch.check_ready_to_start_print()) {
            // Make sure we have the prefetch buffer full before start the print
            // If we got into the "downloading" phase, do the prefetch checking here, because we're waiting for the file to download more
            marlin_server::media_prefetch.issue_fetch();

        } else {
            ChangeState(State::loading);
        }
        break;

    case State::loading:
        if (gcode_info_scan::scan_start_result() == gcode_info_scan::ScanStartResult::not_started) {
            // Wait for the gcode scan to start

        } else if (gcode_info.has_error()) {
            ChangeState(State::file_error_wait_user);

        } else if (!gcode_info.can_be_printed()) {
            // The file is not fully downloaded, wait till we have downloaded enough for printing
            ChangeState(State::download_wait);

        } else if (!gcode_info.is_loaded()) {
            // Wait for the gcode info to fully load

        } else if (!marlin_server::media_prefetch.check_ready_to_start_print()) {
            // Make sure we have the prefetch buffer full before start the print
            marlin_server::media_prefetch.issue_fetch();

        } else {
            // We're ready to print now
            ChangeState((skip_if_able > marlin_server::PreviewSkipIfAble::no) ? stateFromSelftestCheck() : State::preview_wait_user);
        }
        break;

    case State::preview_wait_user:
        switch (response) {

        case Response::Continue: // no difference in state machine, some checks will not be run if tools_mapping is possible
        case Response::Print:
        case Response::PRINT:
            ChangeState(stateFromSelftestCheck());
            if (skip_if_able == marlin_server::PreviewSkipIfAble::no) {
                // If print wasn't maked as started immediately, mark it now
                return Result::MarkStarted;
            }
            break;

        case Response::Back:
            ChangeState(State::inactive);
            return Result::Abort;

        default:
            break;
        }

        // Periodically kindly ask the prefetch thread to check if the file is still valid and update GCodeInfo::has_error
        if (ticks_diff(curr_ms, last_still_valid_check_ms) > 1000 && !still_valid_check_job.is_active()) {
            last_still_valid_check_ms = curr_ms;

            still_valid_check_job.issue([](AsyncJobExecutionControl &) {
                GCodeInfo::getInstance().check_still_valid();
            });
        }

        // Still check for file validity - file could be downloaded enough for the info to show,
        // but the transfer might fail and we want to prevent the user from start the print then
        if (gcode_info.has_error()) {
            ChangeState(State::file_error_wait_user);
            break;
        }
        break;

    case State::unfinished_selftest_wait_user:
        switch (response) {

        case Response::Continue:
            ChangeState(stateFromUpdateCheck());
            break;

        case Response::Abort:
            ChangeState(State::inactive);
            return Result::Abort;

        default:
            break;
        }
        break;

    case State::new_firmware_available_wait_user:
        switch (response) {

        case Response::Continue:
            ChangeState(stateFromPrinterCheck());
            break;

        default:
            // TODO this should be handled more generally with a possibility to set timeout for specific state, but this should work for now and is MUCH easier
            if (ticks_ms() >= new_firmware_open_ms + new_firmware_timeout_ms) {
                ChangeState(stateFromPrinterCheck());
            }
            break;
        }
        break;

#if HAS_MMU2() || HAS_TOOLCHANGER()
    case State::tools_mapping_wait_user:
        switch (response) {

        case Response::Back:
            ChangeState(State::inactive);
            tools_mapping_cleanup();
            return Result::Abort;

        case Response::PRINT:
            tools_mapping_cleanup(true);
            ChangeState(State::done);
            break;

        default:
            break;
        }
        break;
#endif

    case State::wrong_printer_wait_user:
    case State::wrong_printer_wait_user_abort:
        switch (response) {

        case Response::PRINT:
            ChangeState(stateFromFilamentPresence());
            break;

        case Response::Abort:
            ChangeState(State::inactive);
            return Result::Abort;

        default:
            break;
        }
        break;

    case State::filament_not_inserted_wait_user:
        switch (response) {

        case Response::FS_disable:
            FSensors_instance().set_enabled_global(false);
            ChangeState(State::checks_done);
            break;

        case Response::No:
            ChangeState(State::inactive);
            return Result::Abort;

        case Response::Yes:
            ChangeState(State::filament_not_inserted_load);
            queue_filament_load_gcodes();
            break;

        default:
            break;
        }
        break;

    case State::filament_not_inserted_load:
        if (!filament_gcodes::InProgress::Active()) {
            ChangeState(stateFromFilamentType());
        }
        break;

#if HAS_MMU2()
    case State::mmu_filament_inserted_wait_user:
        switch (response) {

        case Response::No:
            ChangeState(State::inactive);
            return Result::Inactive;

        case Response::Yes:

            ChangeState(State::mmu_filament_inserted_unload);
            marlin_server::enqueue_gcode("M702 W0"); // unload, no return or cooldown
            break;

        default:
            break;
        }
        break;

    case State::mmu_filament_inserted_unload:
        if (!filament_gcodes::InProgress::Active()) {
            ChangeState(State::checks_done);
        }
        break;
#endif

    case State::wrong_filament_wait_user: // change / ignore / abort
        switch (response) {

        case Response::Change:
            ChangeState(State::wrong_filament_change);
            queue_filament_change_gcodes();
            break;

        case Response::Ok:
            ChangeState(State::checks_done);
            break;

        case Response::Abort:
            ChangeState(State::inactive);
            return Result::Abort;

        default:
            break;
        }
        break;

    case State::wrong_filament_change:
        if (!filament_gcodes::InProgress::Active()) {
            PreheatStatus::Result res = PreheatStatus::ConsumeResult();
            if (res == PreheatStatus::Result::Aborted || res == PreheatStatus::Result::DidNotFinish) {
                ChangeState(State::wrong_filament_wait_user); // Return back to wrong filament type dialog
            } else {
                ChangeState(State::checks_done);
            }
        }
        break;

    case State::file_error_wait_user:
        // Only one possible response -> abort
        if (response == Response::Abort) {
            ChangeState(State::inactive);
            return Result::Abort;
        }
        break;

    case State::checks_done:
#if PRINTER_IS_PRUSA_iX
        // We've removed reset_bounding_rect at the end of the print for the iX (in marlin_server.cpp::finalize_print).
        // So now, just to make sure, we reset the bounding rect at the start if we don't see it being set in the gcode.
        // BFW-5085
        if (!GCodeInfo::getInstance().get_bed_preheat_area().has_value()) {
            PrintArea::reset_bounding_rect();
        }
#endif

        if (tools_mapping::is_tool_mapping_possible()) {
#if ENABLED(PRUSA_SPOOL_JOIN) && ENABLED(PRUSA_TOOL_MAPPING)
            if ((skip_if_able >= marlin_server::PreviewSkipIfAble::tool_mapping) && PrintPreview::check_tools_mapping_validity(tool_mapper, spool_join, gcode_info).all_ok()) {
                // we can skip tools mapping if there is not warning/error in global tools mapping
                ChangeState(State::done);
                break;
            }

            ChangeState(State::tools_mapping_wait_user);

            // start preheating bed to save time in absorbing heat
            if (GCodeInfo::getInstance().get_bed_preheat_temp().has_value()) {
                if (GCodeInfo::getInstance().get_bed_preheat_area().has_value()) {
                    PrintArea::set_bounding_rect(GCodeInfo::getInstance().get_bed_preheat_area().value());
                }

                marlin_server::set_target_bed(GCodeInfo::getInstance().get_bed_preheat_temp().value());
            }
            break;
#endif
        }
        // else go to print, checks are done
        ChangeState(State::done);
        break;
    case State::done:
        ChangeState(State::inactive);
        return Result::Print;
    }
    return stateToResult();
}

PrintPreview::Result PrintPreview::stateToResult() const {
    switch (GetState()) {

    case State::init:
    case State::download_wait:
    case State::loading:
        return Result::Wait;

    case State::preview_wait_user:
        return Result::Image;

    case State::unfinished_selftest_wait_user:
    case State::new_firmware_available_wait_user:
    case State::wrong_printer_wait_user:
    case State::wrong_printer_wait_user_abort:
    case State::wrong_filament_change:
    case State::wrong_filament_wait_user:
    case State::filament_not_inserted_load:
    case State::filament_not_inserted_wait_user:
#if HAS_MMU2()
    case State::mmu_filament_inserted_unload:
    case State::mmu_filament_inserted_wait_user:
#endif
    case State::checks_done:
    case State::file_error_wait_user:
        return Result::Questions;

    case State::inactive:
    case State::done:
        return Result::Inactive;

#if HAS_MMU2() || HAS_TOOLCHANGER()
    case State::tools_mapping_wait_user:
        return Result::ToolsMapping;
#endif
    }
    return Result::Inactive;
}

void PrintPreview::Init() {
    ChangeState(State::init);
}

IPrintPreview::State PrintPreview::stateFromSelftestCheck() {
#if (!DEVELOPER_MODE() && HAS_SELFTEST())
    const bool show_warning = !selftest_warning_selftest_finished();
#else
    const bool show_warning = false;
#endif
    if (show_warning) {
        return State::unfinished_selftest_wait_user;
    } else {
        return stateFromUpdateCheck();
    }
}

IPrintPreview::State PrintPreview::stateFromUpdateCheck() {
    if (GCodeInfo::getInstance().get_valid_printer_settings().outdated_firmware.is_valid()) {
        return stateFromPrinterCheck();
    } else {
        new_firmware_open_ms = ticks_ms();
        return State::new_firmware_available_wait_user;
    }
}

IPrintPreview::State PrintPreview::stateFromPrinterCheck() {
    GCodeInfo::getInstance().EvaluateToolsValid(); // Evaluate tool validity after tools mapping is done
    if (GCodeInfo::getInstance().get_valid_printer_settings().is_valid(tools_mapping::is_tool_mapping_possible())) {
        return stateFromFilamentPresence();
    } else {
        return GCodeInfo::getInstance().get_valid_printer_settings().is_fatal(tools_mapping::is_tool_mapping_possible()) ? State::wrong_printer_wait_user_abort : State::wrong_printer_wait_user;
    }
}
