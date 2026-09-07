#include "config_features.h"
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_mmu2.h>
#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"
#include "pause_stubbed.hpp"
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include "M70X.hpp"
#include <option/has_toolchanger.h>
#include <tool_index.hpp>
#include <utils/overloaded_visitor.hpp>
#include <tool_index_iterator.hpp>
#include <fsm/preheat_phases.hpp>
#include <utils/variant_utils.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

#include <option/has_ht_hotend.h>
#if HAS_HT_HOTEND()
    #include <marlin_server.hpp>
#endif

#include <option/has_anfc.h>
#include <bsod/bsod.h>
#if HAS_ANFC()
    #include <feature/openprinttag/tool_tag.hpp>
    #include <feature/openprinttag/data_utils.hpp>
    #include <feature/openprinttag/requests_read_multi.hpp>
#endif

#if HAS_ANFC()
static bool can_use_openprinttag(PreheatMode preheat_mode) {
    switch (preheat_mode) {

    case PreheatMode::autoload:
    case PreheatMode::standard_load:
        // This is when we want to be smart
        return true;

    case PreheatMode::change_load:
        // During filament change, it is likely that there will still be the unloaded spool at the reader
        // Trying to be smart here might actually be dumb.
        return false;

    case PreheatMode::preheat:
        // User is currently always prompted for the filament parameters on preheat, don't change this behvavior
        return false;

    case PreheatMode::unload:
    case PreheatMode::purge:
        // At this point, filament parameters should be known.
        // If the user needs to load them from a tag, they can still do it manually from the menu.
        return false;
    }

    bsod_unreachable();
}
#endif

PreheatData filament_gcodes::FilamentSelectionArgs::fsm_data() const {
    return PreheatData {
        .tool = tool,
        .mode = mode,
        .has_return_option = bool(std::to_underlying(ret_cool) & std::to_underlying(RetAndCool_t::Return)),
        .has_cooldown_option = bool(std::to_underlying(ret_cool) & std::to_underlying(RetAndCool_t::Cooldown)),
    };
}

static FSMResponseVariant determine_filament_for_operation(const filament_gcodes::FilamentSelectionArgs &preheat_data) {
    const auto serialized_data = preheat_data.fsm_data().serialize();

    const auto deduced_filament_type = [&] {
        if (preheat_data.disregard_loaded_filament) {
            return FilamentType::none;
        }

        if ((preheat_data.mode != PreheatMode::unload) && (preheat_data.mode != PreheatMode::purge)) {
            // We cannot know the temperature, and thus must ask the user
            return FilamentType::none;
        }

        return match(
            preheat_data.tool, //
            [](VirtualToolIndex i) { return config_store().get_filament_type(i); }, //
            [](AllTools) { return FilamentType::none; });
    }();

    if (deduced_filament_type != FilamentType::none) {
        // We know the filament parameters, no need to ask the user
        return FSMResponseVariant::make(deduced_filament_type);
    }

    marlin_server::FSM_Holder fsm { ClientFSM::Preheat };

    const auto check_early_exit = [&] -> std::optional<FSMResponseVariant> {
        if (preheat_data.mode == PreheatMode::autoload && FSensors_instance().sensor_state(LogicalFilamentSensor::primary_runout) == FilamentSensorState::NoFilament) {
            return FSMResponseVariant::make(Response::Abort);
        }

        if (preheat_data.mode == PreheatMode::preheat && FSensors_instance().IsAutoloadInProgress()) {
            return FSMResponseVariant();
        }

        if (gcode_exceptions().is_unwinding()) {
            return FSMResponseVariant::make(Response::Abort);
        }

        return std::nullopt;
    };

#if HAS_ANFC()
    const auto tool = stdext::get_optional<VirtualToolIndex>(preheat_data.tool);
    const auto tag = tool.and_then(buddy::openprinttag::ToolTag::for_tool_ephemeral);

    // If an OPT is detected for the tool, possibly automatically load data from it
    if (can_use_openprinttag(preheat_data.mode) && tag.has_value()) {
        namespace opt = buddy::openprinttag;

        Tristate load_tag = config_store().opt_auto_read_on_load.get();

        if (preheat_data.openprinttag_uid_hash == tag->uid_hash()) {
            load_tag = Tristate::yes;
        }

        if (load_tag == Tristate::other) {
            // Ask the user if they want to load the data
            fsm.change(PhasesPreheat::ask_load_openprinttag, serialized_data);
            while (true) {
                switch (marlin_server::get_response_from_phase(PhasesPreheat::ask_load_openprinttag)) {

                case Response::Yes:
                    load_tag = Tristate::yes;
                    goto break_ask_load_opt_loop;

                case Response::No:
                    load_tag = Tristate::no;
                    goto break_ask_load_opt_loop;

                case Response::Always:
                    load_tag = Tristate::yes;
                    config_store().opt_auto_read_on_load.set(true);
                    goto break_ask_load_opt_loop;

                case Response::Never:
                    load_tag = Tristate::no;
                    config_store().opt_auto_read_on_load.set(false);
                    goto break_ask_load_opt_loop;

                case Response::_none:
                    break;

                default:
                    bsod_unreachable();
                }

                if (auto e = check_early_exit()) {
                    return *e;
                }

                idle(true);
            }
        break_ask_load_opt_loop:
        }

        if (load_tag == Tristate::yes) {
            opt::MultiReadFieldRequest<opt::FilamentParametersInfo::Requirements {}> req { *tag };
            req.issue();

            while (!req.finished()) {
                if (auto e = check_early_exit()) {
                    return *e;
                }

                idle(true);
            }

            opt::FilamentParametersInfo params { req };

            if (params.data_safe_to_use) {
                // Everything derived perfectly from the tag -> apply the parameters and be happy
                const FilamentType ft = PendingAdHocFilamentType {};
                ft.set_parameters(params.parameters);
                return FSMResponseVariant::make(ft);

            } else {
                // The printer was not able to derive all the necessary fields
                // -> pop up ScreenOPTFilamentDetail and let the user tweak the settings
                // unknown fields will be highlighted
                // (the screen loads the OPT data again on the GUI thread)

                fsm.change(PhasesPreheat::openprinttag_parameters, serialized_data);

                // Wait for the ScreenOPTFilamentDetail to open and then switch to user_temp_selection phase
                // See hack explanation in PhasesPreheat::openprinttag_parameters doxygen
                while (true) {
                    switch (marlin_server::get_response_from_phase(PhasesPreheat::openprinttag_parameters)) {

                    case Response::Ok:
                        goto break_opt_params_loop;

                    case Response::_none:
                        break;

                    default:
                        bsod_unreachable();
                    }

                    if (auto e = check_early_exit()) {
                        return *e;
                    }

                    idle(true);
                }
            break_opt_params_loop:
            }
        }
    }
#endif

    fsm.change(PhasesPreheat::user_temp_selection, serialized_data);

    while (true) {
        if (const auto ret = marlin_server::get_response_variant_from_phase(PhasesPreheat::user_temp_selection)) {
            return ret;
        }

        if (auto e = check_early_exit()) {
            return *e;
        }

        idle(true);
    }
}

std::pair<std::optional<PreheatStatus::Result>, FilamentType> filament_gcodes::preheat(const FilamentSelectionArgs &preheat_data, PreheatBehavior preheat_arg) {
    const FSMResponseVariant response = determine_filament_for_operation(preheat_data);

    const auto physical_tool = to_physical_tool_index<AllTools>(preheat_data.tool);

    if (response.holds_alternative<FilamentType>()) {
        const FilamentType filament = response.value<FilamentType>();
        preheat_to(filament, physical_tool, preheat_arg);
        return { std::nullopt, filament };
    }

    switch (response.value_or<Response>(Response::_none)) {

    case Response::Abort:
        return { PreheatStatus::Result::Aborted, FilamentType::none };

    case Response::Cooldown:
        return { PreheatStatus::Result::CooledDown, FilamentType::none };

    default:
        // should not happen
        return { PreheatStatus::Result::Error, FilamentType::none };
    }
}

filament_gcodes::PreheatBehavior filament_gcodes::PreheatBehavior::for_filament_load(bool force_temp) {
    const bool preheat_all = config_store().filament_change_preheat_all.get();

    return PreheatBehavior {
        .force_temp = force_temp,
        .preheat_bed = preheat_all,
#if HAS_CHAMBER_API()
        .set_chamber_temperature = preheat_all,
#endif
        .consider_previous_filament = true,
    };
}

filament_gcodes::PreheatBehavior filament_gcodes::PreheatBehavior::for_filament_unload(bool force_temp) {
    return PreheatBehavior {
        .force_temp = force_temp,
        .preheat_bed = false,
#if HAS_CHAMBER_API()
        .set_chamber_temperature = false,
#endif
        .consider_previous_filament = false,
    };
}

void filament_gcodes::preheat_to(FilamentType filament, std::variant<PhysicalToolIndex, AllTools> tools, PreheatBehavior preheat_arg) {
    const FilamentTypeParameters fil_cnf = filament.parameters();

    bool hotend_temp_changed = false;

    // change temp only if it is lower than currently loaded filament
    // TODO: Why? This is now very problematic with the new heatbreak and chamber params
    for (const PhysicalToolIndex tool : tool_index_iterator(tools)) {
        int16_t target_temp = fil_cnf.nozzle_temperature;

        if (const auto virtual_tool = stdext::get_optional<VirtualToolIndex>(tool.currently_selected_virtual_tool())) {
            // If the previously loaded filament had a higher nozzle temperature,
            // preheat to that temperature to ensure the remnants in the nozzle
            // will melt enough to allow the load.
            //
            // If the previously loaded filament is unknown, preheat to 250C
            // for the case there was a high-temperature filament in the nozzle
            // previously.
            //
            // Only do this if there's no currently loaded filament to avoid
            // preheating to 250C if we're already loaded.
            if (preheat_arg.consider_previous_filament && !config_store().get_filament_type(*virtual_tool)) {
                int16_t prev_temp = 250; // default previous filament temperature in case the previous filament is unknown
                const FilamentType prev_filament = config_store().get_previous_filament_type(*virtual_tool);
                if (prev_filament != FilamentType::none) {
                    prev_temp = prev_filament.parameters().nozzle_temperature;
                }
                target_temp = std::max(target_temp, prev_temp);
            }
        }

        if (preheat_arg.force_temp || thermalManager.degTargetHotend(tool) < target_temp) {
            hotend_temp_changed = true;
            thermalManager.setTargetHotend(target_temp, tool);
        }

#if HAS_FILAMENT_HEATBREAK_PARAM()
        thermalManager.setTargetHeatbreak(fil_cnf.heatbreak_temperature, tool);
#endif
    }

    if (hotend_temp_changed && preheat_arg.preheat_bed && (preheat_arg.force_temp || (thermalManager.degTargetBed() < fil_cnf.heatbed_temperature))) {
        thermalManager.setTargetBed(fil_cnf.heatbed_temperature);
    }

#if HAS_CHAMBER_API()
    if (preheat_arg.set_chamber_temperature) {
        buddy::chamber().set_target_temperature(fil_cnf.chamber_target_temperature);
    }
#endif
}

void filament_gcodes::M1700_preheat(const M1700Args &args) {
    InProgress progress;
    const FSMResponseVariant response_variant = determine_filament_for_operation(FilamentSelectionArgs {
        .mode = args.mode,
        .tool = args.tool,
        .ret_cool = args.preheat,
        .disregard_loaded_filament = true,
    });
    // autoload ocurred
    if (!response_variant) {
        return;
    }

    const Response response = response_variant.value_or<Response>(Response::_none);
    if (response == Response::Abort) {
        PreheatStatus::SetResult(PreheatStatus::Result::Aborted);
        return;
    }

    const FilamentType filament = response_variant.value_or<FilamentType>(FilamentType::none);
    const FilamentTypeParameters fil_cnf = filament.parameters();

    auto iterator = tool_index_iterator(args.tool);
    if (response == Response::Cooldown) {
        // Cooldown applies for all tools
        iterator = VirtualToolIndex::Iterator::make_all();
    }
    iterator = iterator.skip_all_disabled();

    for (VirtualToolIndex virtual_tool : iterator) {
        const PhysicalToolIndex physical_tool = virtual_tool.to_physical();
        thermalManager.setTargetHotend(args.enforce_target_temp ? fil_cnf.nozzle_temperature : fil_cnf.nozzle_preheat_temperature, physical_tool);
#if HAS_FILAMENT_HEATBREAK_PARAM()
        if (args.set_heatbreak) {
            thermalManager.setTargetHeatbreak(fil_cnf.heatbreak_temperature, physical_tool);
        }
#endif
    }

    if (args.preheat_bed) {
        thermalManager.setTargetBed(fil_cnf.heatbed_temperature);
    }

#if HAS_CHAMBER_API()
    if (args.preheat_chamber) {
        buddy::chamber().set_target_temperature(fil_cnf.chamber_target_temperature);
    #if HAS_CHAMBER_VENTS()
        buddy::chamber().manage_ventilation_state(fil_cnf.chamber_target_temperature);
    #endif
    }
#endif

    // cooldown pressed
    if (filament == FilamentType::none) {
        thermalManager.set_print_fan_speed(0);

    } else if (!axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
        unhomed_z_lift(10);
    }

    if (args.save && response != Response::Cooldown) {
        for (const VirtualToolIndex tool : iterator) {
            config_store().set_filament_type(tool, filament);
        }
    }

    // store result, so other threads can see it
    PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);

    // we might want to set filament type even with preheat, if so do:
    // Filaments::SetToBeLoaded(filament);
}
