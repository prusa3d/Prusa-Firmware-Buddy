/**
 * @file marlin_print_preview.cpp
 */

#include "marlin_print_preview.hpp"
#include "client_fsm_types.h"
#include "client_response.hpp"
#include "general_response.hpp"
#include "marlin_server.hpp"
#include "timing.h"
#include "filament_sensors_handler.hpp"
#include "filament.hpp"
#include "M70X.hpp"
#include <option/developer_mode.h>
#include "printers.h"
#include <Marlin/src/module/motion.h>
#include "screen_menu_filament_changeall.hpp"
#include "box_unfinished_selftest.hpp"

#include <option/has_toolchanger.h>
#if ENABLED(PRUSA_TOOLCHANGER)
    #include <module/prusa/toolchanger.h>
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/

#include <config_store/store_instance.hpp>

// would be nice to have option leave phase as it was
// something like std::pair<enum {delete, leave, has_value },PhasesPrintPreview>
// but not currently needed
std::optional<PhasesPrintPreview> IPrintPreview::getCorrespondingPhase(IPrintPreview::State state) {
    switch (state) {
    case State::inactive:
        return std::nullopt;

    case State::preview_wait_user:
        return PhasesPrintPreview::main_dialog;

    case State::unfinished_selftest_wait_user:
        return PhasesPrintPreview::unfinished_selftest;

    case State::new_firmware_available_wait_user:
        return PhasesPrintPreview::new_firmware_available;
    case State::tools_mapping_wait_user:
    case State::tools_mapping_change:
        return PhasesPrintPreview::tools_mapping;

    case State::wrong_printer_wait_user:
        return PhasesPrintPreview::wrong_printer;

    case State::wrong_printer_wait_user_abort:
        return PhasesPrintPreview::wrong_printer_abort;

    case State::filament_not_inserted_wait_user:
    case State::filament_not_inserted_load:
        return PhasesPrintPreview::filament_not_inserted;

    case State::mmu_filament_inserted_wait_user:
    case State::mmu_filament_inserted_unload:
        return PhasesPrintPreview::mmu_filament_inserted;

    case State::wrong_filament_wait_user:
    case State::wrong_filament_change:
        return PhasesPrintPreview::wrong_filament;

    case State::done:
        return std::nullopt;
    }
    return std::nullopt;
}

void IPrintPreview::ChangeState(State s) {
    state = s;
    setFsm(getCorrespondingPhase(state));
}

void IPrintPreview::setFsm(std::optional<PhasesPrintPreview> wantedPhase) {
    FSM_action action = IsFSM_action_needed(phase, wantedPhase);
    switch (action) {
    case FSM_action::no_action:
        break;
    case FSM_action::create:
        if (wantedPhase && *wantedPhase != PhasesPrintPreview::_first) {
            FSM_CREATE_WITH_DATA__LOGGING(PrintPreview, *wantedPhase, fsm::PhaseData({ 0, 0, 0, 0 }));
        } else {
            FSM_CREATE__LOGGING(PrintPreview);
        }
        break;
    case FSM_action::destroy:
        // do not call FSM_DESTROY__LOGGING(PrintPreview);
        // we need to call it manually later to be atomic
        break;
    case FSM_action::change:
        FSM_CHANGE__LOGGING(PrintPreview, *wantedPhase); // wantedPhase is not nullopt, FSM_action would not be change otherwise
        break;
    }
    phase = wantedPhase;
}

Response IPrintPreview::GetResponse() {
    return phase ? marlin_server::ClientResponseHandler::GetResponseFromPhase(*phase) : Response::_none;
}

static bool is_same(const char *curr_filament, const GCodeInfo::filament_buff &filament_type) {
    return strncmp(curr_filament, filament_type.begin(), filament_type.size()) == 0;
}
static bool filament_known(const char *curr_filament) {
    return strncmp(curr_filament, "---", 3) != 0;
}

static bool check_tool_need_filament_load(uint8_t tool) {
    // unused tool doesn't need filament load
    if (!GCodeInfo::getInstance().get_extruder_info(tool).used())
        return false;

    // when tool doesn't have filament, it needs load
    return !FSensors_instance().ToolHasFilament(tool);
}

static bool check_correct_filament_type(uint8_t tool) {
    const auto &extruder_info = GCodeInfo::getInstance().get_extruder_info(tool);
    if (!extruder_info.used())
        return true; // when tool not used in print, return OK filament type

    if (!extruder_info.filament_name.has_value())
        return true; // filament type unspecified, return tool OK

    const auto loaded_filament_type = config_store().get_filament_type(tool);
    const auto loaded_filament_desc = filament::get_description(loaded_filament_type);
    // when loaded filament type not known, return that filament type is OK
    return !filament_known(extruder_info.filament_name.value().data()) || is_same(loaded_filament_desc.name, extruder_info.filament_name.value());
}

IPrintPreview::State PrintPreview::stateFromFilamentPresence() const {

    if (FSensors_instance().HasMMU()) {
        if (!config_store().fsensor_enabled.get()) {
            return State::done;
        }
        // with MMU, its only possible to check that filament is properly unloaded, no check of filaments presence in each "tool"
        if (FSensors_instance().MMUReadyToPrint()) {
            return State::done;
        } else {
            return State::mmu_filament_inserted_wait_user;
        }
    } else {
        if (!config_store().fsensor_enabled.get()) {
            return stateFromFilamentType();
        }

        // no MMU, do regular check of filament presence in each tool
        HOTEND_LOOP() {
            if (check_tool_need_filament_load(e)) {
                return State::filament_not_inserted_wait_user;
            }
        }
        return stateFromFilamentType();
    }
}

static void queue_filament_load_gcodes() {
    // Queue load filament gcode for every tool that doesn't have filament loaded
    HOTEND_LOOP() {
        // skip for tools that already have filament
        if (!check_tool_need_filament_load(e))
            continue;

        // pass filament type from gcode, so that user doesn't have to select filament type
        const char *filament_name = GCodeInfo::getInstance().get_extruder_info(e).filament_name.has_value() ? &GCodeInfo::getInstance().get_extruder_info(e).filament_name.value()[0] : "";
#if HOTENDS > 1
        // if printer has multiple hotends (eg: XL), preheat all that will be loaded to save time for user
        auto target_temp = filament::get_description(filament::get_type(filament_name, strlen(filament_name))).nozzle;
        thermalManager.setTargetHotend(target_temp, e);
        marlin_server::set_temp_to_display(target_temp, e);
#endif
        marlin_server::enqueue_gcode_printf("M701 S\"%s\" T%d W2", filament_name, e);
    }
}

static void queue_filament_change_gcodes() {
    // Queue change filament gcode for every tool with mismatched filament type
    HOTEND_LOOP() {
        if (!check_correct_filament_type(e)) {
            // pass filament type from gcode, so that user doesn't have to select filament type
            const char *filament_name = GCodeInfo::getInstance().get_extruder_info(e).filament_name.has_value() ? &GCodeInfo::getInstance().get_extruder_info(e).filament_name.value()[0] : "";

#if HOTENDS > 1
            // if printer has multiple hotends (eg: XL), preheat all that will be loaded to save time for user
            auto temp_old = filament::get_description(config_store().get_filament_type(e)).nozzle;

            thermalManager.setTargetHotend(temp_old, e);
            marlin_server::set_temp_to_display(temp_old, e);
#endif

            // M1600 - change, R - add return option, U1 - Ask filament type if unknown, T - tool, Sxxx - preselect filament type
            marlin_server::enqueue_gcode_printf("M1600 S\"%s\" T%d R U1", filament_name, e);
        }
    }
}

IPrintPreview::State PrintPreview::stateFromFilamentType() const {
    // Check match of loaded and G-code types
    HOTEND_LOOP() {
        if (!check_correct_filament_type(e)) {
            return State::wrong_filament_wait_user;
        }
    }
    return State::done;
}

PrintPreview::Result PrintPreview::Loop() {
    if (GetState() == State::inactive)
        return Result::Inactive;

    uint32_t time = ticks_ms();
    if ((time - last_run) < max_run_period_ms)
        return stateToResult();
    last_run = time;
    const Response response = GetResponse();

    switch (GetState()) {
    case State::inactive: // cannot be, but have it defined to enumerate all states
        return Result::Inactive;
    case State::preview_wait_user:
        switch (response) {
        case Response::Continue:
#if HAS_TOOLCHANGER()
            if (!(GCodeInfo::getInstance().UsedExtrudersCount() == 1 && prusa_toolchanger.get_num_enabled_tools() == 1)) {
                ChangeState(State::tools_mapping_wait_user);
                break;
            }
#endif
            [[fallthrough]]; // else we have only 1-1 available, so do normal printing
        case Response::Print:
            ChangeState(stateFromSelftestCheck());
            break;
        case Response::Back:
            ChangeState(State::inactive);
            return Result::Abort;
        default:
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
    case State::tools_mapping_wait_user:
    case State::tools_mapping_change:
        switch (response) {
        case Response::Back:
            ChangeState(State::inactive);
            return Result::Abort;
        case Response::PRINT:
            ChangeState(stateFromPrinterCheck());
            break;
        case Response::Change:
            ChangeState(State::tools_mapping_change);
            marlin_server::enqueue_gcode("M1600 R"); // TODO change, return option
            break;
        default:
            break;
        }
        break;
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
            FSensors_instance().Disable();
            ChangeState(State::done);
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

    case State::mmu_filament_inserted_wait_user:
        switch (response) {
        case Response::No:
            ChangeState(State::inactive);
            return Result::Inactive;
        case Response::Yes:
            ChangeState(State::mmu_filament_inserted_unload);
            marlin_server::enqueue_gcode("M702 W0"); // load, no return or cooldown
            break;
        default:
            break;
        }
        break;

    case State::mmu_filament_inserted_unload:
        if (!filament_gcodes::InProgress::Active()) {
            ChangeState(State::done);
        }
        break;

    case State::wrong_filament_wait_user: // change / ignore / abort
        switch (response) {
        case Response::Change:
            ChangeState(State::wrong_filament_change);
            queue_filament_change_gcodes();
            break;
        case Response::Ok:
            ChangeState(State::done);
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
                ChangeState(State::done);
            }
        }
        break;

    case State::done:
        ChangeState(State::inactive);
        return Result::Print;
    }
    return stateToResult();
}

PrintPreview::Result PrintPreview::stateToResult() const {
    switch (GetState()) {
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
    case State::mmu_filament_inserted_unload:
    case State::mmu_filament_inserted_wait_user:
        return Result::Questions;
    case State::inactive:
    case State::done:
        return Result::Inactive;
    case State::tools_mapping_wait_user:
    case State::tools_mapping_change:
        return Result::ToolsMapping;
    }
    return Result::Inactive;
}

void PrintPreview::Init() {
    ChangeState(skip_if_able ? stateFromSelftestCheck() : State::preview_wait_user);
}

IPrintPreview::State PrintPreview::stateFromSelftestCheck() {
#if (!DEVELOPER_MODE() && PRINTER_IS_PRUSA_XL)
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
    if (GCodeInfo::getInstance().get_valid_printer_settings().is_valid()) {
        return stateFromFilamentPresence();
    } else {
        return GCodeInfo::getInstance().get_valid_printer_settings().is_fatal() ? State::wrong_printer_wait_user_abort : State::wrong_printer_wait_user;
    }
}
