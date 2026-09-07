#include "store_definition.hpp"
#include <common/visit_all_struct_fields.hpp>
#include <Marlin/src/inc/MarlinConfigPre.h>
#include <module/prusa/dock_position.hpp>
#include <module/prusa/tool_offset.hpp>
#include <option/has_adc_side_fsensor.h>
#include <option/has_toolchanger.h>
#include <option/has_config_store_wo_backend.h>
#include <option/has_touch.h>
#include <option/has_chamber_filtration_api.h>
#include <common/sys.hpp>
#include <common/printer_variant/printer_variant.hpp>

#include <option/has_selftest.h>
#if HAS_SELFTEST()
    #include <selftest_result.hpp>
#endif

#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_anfc.h>
#include <bsod/bsod.h>
#if HAS_ANFC()
    #include <feature/openprinttag/tool_tag.hpp>
#endif

namespace config_store_ns {
#if not HAS_CONFIG_STORE_WO_BACKEND()
static_assert((sizeof(CurrentStore) + aggregate_arity<CurrentStore>() * sizeof(journal::Backend::ItemHeader)) < (BANK_SIZE / 100) * 75, "EEPROM bank is almost full");
static_assert(journal::has_unique_items<config_store_ns::CurrentStore>(), "Just added items are causing collisions with reserved backend IDs");
static_assert(aggregate_arity<config_store_ns::CurrentStore>() > 10, "Config store sanity check failed");
static_assert(
    ![] {
        bool is_problem = false;
        CurrentStore s {};
        visit_all_struct_fields(s, [&is_problem]<typename T>(T &) {
            if constexpr ((T::flags & ~(ItemFlag::dev_items | ItemFlag::common_misconfigurations)) == 0) {
                is_problem = true;
            }
        });
        return is_problem; //
    }(),
    "All items must have a flag set (not counting dev_items/common_misconfigurations)");
#endif

void CurrentStore::perform_config_check() {
    /// Whether this is the first run of the printer after assembly/factory reset
    [[maybe_unused]] const bool is_first_run = (config_store_init_result() == InitResult::cold_start);

    // We cannot change a default value of config store items for backwards compatibility reasons.
    // So this is a place to instead set them to something for new installations
    if (is_first_run || force_default_hw_config.get()) {
        force_default_hw_config.set(false);

#if PRINTER_IS_PRUSA_MK4()
        change_extended_printer_type(PrinterModel::mk4s, ChangeExtendedPrinterTypeMode::config_store_init);
        hotend_type.set(0, HotendType::stock_with_sock);
        nozzle_is_high_flow.set(1 << 0); // Bitset -> first and only nozzle

#elif PRINTER_IS_PRUSA_XL()
        change_extended_printer_type(PrinterModel::xls, ChangeExtendedPrinterTypeMode::config_store_init);

        for (auto tool : PhysicalToolIndex::all()) {
            set_nozzle_diameter(tool, 0.4f); // New XL printers have .4mm nozzles: BFW-5638
            hotend_type.set(tool.to_raw(), HotendType::stock_with_sock); // New printers also sock
        }

#elif PRINTER_IS_PRUSA_MK3_5()
        change_extended_printer_type(PrinterModel::mk3_5s, ChangeExtendedPrinterTypeMode::config_store_init);

#endif

#if HAS_PRINTER_VARIANT()
        // The restart-required result can be ignored: this runs at boot, before the planner reads steps/mm.
        (void)apply_printer_variant_defaults(printer_variant_after_factory_reset);
#endif
    }

#if HAS_CHAMBER_FILTRATION_API()
    // Old API had disabling print filtration through setting pwm to 0
    // Now we have dedicated config store for it
    if (chamber_mid_print_filtration_pwm.get() <= PWM255(0)) {
        chamber_mid_print_filtration_pwm.set_to_default();
        chamber_print_filtration_enable.set(false);
    }
#endif

    // BFW-5486
    // Older versions of the firmware had the ability to manually change this
    // byte. Newer versions of the firmware removed that ability. This leads
    // to a situation when, after manually changing the value and upgrading,
    // the only way to revert the change is to downgrade the firmware.
    // Therefore, we always set it to FwAutoUpdate::off on newer versions.
    // We should update the bootloader to stop reading this byte altogether,
    // then we can finally stop writing this and rely entirely on dataexchange.
    uint8_t null_byte = 0x00;
    EEPROMInstance().write_bytes(0x040B, trivial_as_bytes(null_byte));
    EEPROMInstance().flush();

    // First run -> the config store is empty -> we don't need to do any migrations from older versions
    if (!is_first_run && config_version.get() != newest_config_version) {
        perform_config_migrations();
    }

    config_version.set(newest_config_version);
}

namespace {
    template <size_t new_version>
    bool should_migrate() {
        static_assert(CurrentStore::newest_config_version >= new_version);
        return config_store().config_version.get() < new_version;
    }
} // namespace

void CurrentStore::perform_config_migrations() {
    // See the comment on the bottom of this function

#if PRINTER_IS_PRUSA_MK4()
    if (should_migrate<1>()) {
        // We've introduced nozzle_is_high_flow in 6.2.0
        // If the user upgrades from previous FW versions, we need to guess the HF nozleness based on whether he has MK4S or not
        // MK4S+MMU is shipped and recommended without the HF nozzle, so exclude those

        const auto model = PrinterModelInfo::current().model;
        if ((model == PrinterModel::mk4s || model == PrinterModel::mk3_9s) && !is_mmu_rework.get()) {
            // Bitset -> first and only nozzle
            nozzle_is_high_flow.set(1 << 0);
        }
    }
#endif

#if HAS_SELFTEST() && (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_COREONE())
    if (should_migrate<2>()) {
        // We've introduced a gearbox alignment for XL, this means that gear alignment test must exist for every toolhead available
        // This created a need for gear test refactoring
        // [[ BFW-5785 ]]

        auto sr = selftest_result.get();
        sr.set_gearbox(0, sr.get_deprecated_gears());
        selftest_result.set(sr);
    }
#endif
#if HAS_SELFTEST()
    if (should_migrate<3>()) {
        // BFW-7867
        // Do not show pritner setup screen if the user has run any selftests
        // This is for backwards compatibility - we don't want to show the screen after the firmware update introducing it for already configured printers
        if (selftest_result.get() != selftest_result.default_val) {
            printer_hw_config_done.set(true);
            printer_network_setup_done.set(true);
        }
    }
#endif
    if (should_migrate<4>()) {
        // Don't show "Happy Printing" screen when upgrading firmware
        happy_printing_seen.set(true);
    }
#if PRINTER_IS_PRUSA_COREONE()
    if (should_migrate<5>()) {
        // Printers that are upgraded are most likely CoreOne without vent grille lever
        // Those have to keep the manual open/close mechanism.
        // On new CoreOne+ printers, this will be checked in HW config
        auto_chamber_vent_enabled.set(false);
    }
#endif

#if HAS_INDX()
    if (should_migrate<6>()) {
        // BFW-9018
        // The X calibration measures the nozzle cleaner V-groove, but the nominal reference
        // (X_NOZZLE_CLEANER_ORIGIN) used to point 0.65 mm away from it, so every stored offset
        // has +0.65 baked in. This FW moves the reference onto the V-groove, so remove it.
        // Uncalibrated printers keep the default offset untouched.
        if (selftest_result_nozzle_cleaner_calibration.get() == TestResult::passed) {
            nozzle_cleaner_x_origin_offset.set(nozzle_cleaner_x_origin_offset.get() - 0.65f);
        }
    }
#endif

    // To add a migration:
    // - increment newest_config_version
    // - add if(should_migrate<X>) { your migration code } at the END of this function
    //    - the migrations have to be in an increasing order
    //    - the X shall be the new incremented newest_config_version value
    // - keep this comment on the BOTTOM of this function, so that it's visible when reviewing every new migration
    //
    // Don't confuse this with the config_store migrations.
    // - config_store migrations are migrations on store item level (when the item structure changes and so on). They do not have access to the whole config_store/printer state.
    // - migrations here are for the higher abstraction level
}

int32_t CurrentStore::get_extruder_fs_ref_nins_value([[maybe_unused]] uint8_t index) {
    return extruder_fs_ref_nins_values.get(index);
}

void CurrentStore::set_extruder_fs_ref_nins_value([[maybe_unused]] uint8_t index, int32_t value) {
    extruder_fs_ref_nins_values.set(index, value);
}

int32_t CurrentStore::get_extruder_fs_ref_ins_value([[maybe_unused]] uint8_t index) {
    return extruder_fs_ref_ins_values.get(index);
}

void CurrentStore::set_extruder_fs_ref_ins_value([[maybe_unused]] uint8_t index, int32_t value) {
    extruder_fs_ref_ins_values.set(index, value);
}

#if HAS_ADC_SIDE_FSENSOR()
int32_t CurrentStore::get_side_fs_ref_nins_value(uint8_t index) {
    return side_fs_ref_nins_values.get(index);
}

void CurrentStore::set_side_fs_ref_nins_value(uint8_t index, int32_t value) {
    side_fs_ref_nins_values.set(index, value);
}

int32_t CurrentStore::get_side_fs_ref_ins_value(uint8_t index) {
    return side_fs_ref_ins_values.get(index);
}

void CurrentStore::set_side_fs_ref_ins_value(uint8_t index, int32_t value) {
    side_fs_ref_ins_values.set(index, value);
}
#endif

FilamentType CurrentStore::get_filament_type([[maybe_unused]] uint8_t index) {
    if (loaded_filament_is_previous.get()[index]) {
        return FilamentType::none;
    }
    return loaded_filament_type.get(index);
}

void CurrentStore::set_filament_type(VirtualToolIndex virtual_tool, FilamentType value) {
    if (value == PendingAdHocFilamentType {}) {
        const FilamentType new_value = AdHocFilamentType { .tool = virtual_tool.to_raw() };
        new_value.set_parameters(value.parameters());

#if HAS_ANFC()
        value.modify_parameters([](FilamentTypeParameters &p) {
            // Clear OPT link so that it's not accidentally reused
            p.openprinttag_uid_hash = buddy::openprinttag::ToolTag::no_tag_hash;
        });
#endif

        value = new_value;
    }

    if (value == FilamentType::none) {
#if HAS_AUTO_RETRACT()
        // On filament removal, it invalidates retracted distance
        buddy::auto_retract().set_retracted_distance(virtual_tool.to_physical(), std::nullopt);
#endif

        loaded_filament_is_previous.apply([&](auto &item) {
            item.set(virtual_tool.to_raw(), true);
        });
    } else {
        loaded_filament_type.set(virtual_tool.to_raw(), value);
        loaded_filament_is_previous.apply([&](auto &item) {
            item.set(virtual_tool.to_raw(), false);
        });
    }
}

FilamentType CurrentStore::get_previous_filament_type(VirtualToolIndex tool) {
    if (loaded_filament_is_previous.get()[tool.to_raw()]) {
        return loaded_filament_type.get(tool.to_raw());
    } else {
        return FilamentType::none;
    }
}

void CurrentStore::clear_previous_filament_type(uint8_t index) {
    if (loaded_filament_is_previous.get()[index]) {
        loaded_filament_type.set(index, FilamentType::none);
    }
}

float CurrentStore::get_nozzle_diameter([[maybe_unused]] uint8_t index) {
    return nozzle_diameters.get(index);
}

void CurrentStore::set_nozzle_diameter([[maybe_unused]] uint8_t index, float value) {
    nozzle_diameters.set(index, value);
    clear_previous_filament_type(index);
}

bool CurrentStore::get_nozzle_is_hardened([[maybe_unused]] uint8_t index) {
    return nozzle_is_hardened.get()[index];
}

void CurrentStore::set_nozzle_is_hardened([[maybe_unused]] uint8_t index, bool value) {
    nozzle_is_hardened.apply([&](auto &item) {
        item.set(index, value);
    });
    clear_previous_filament_type(index);
}

bool CurrentStore::get_nozzle_is_high_flow([[maybe_unused]] uint8_t index) {
    return nozzle_is_high_flow.get()[index];
}

void CurrentStore::set_nozzle_is_high_flow([[maybe_unused]] uint8_t index, bool value) {
    nozzle_is_high_flow.apply([&](auto &item) {
        item.set(index, value);
    });
    clear_previous_filament_type(index);
}

float CurrentStore::get_odometer_axis(uint8_t index) {

    switch (index) {
    case 0:
        return odometer_x.get();
    case 1:
        return odometer_y.get();
    case 2:
        return odometer_z.get();
    default:
        debug_assert(false && "invalid index");
        return {};
    }
}

void CurrentStore::set_odometer_axis(uint8_t index, float value) {
    switch (index) {
    case 0:
        odometer_x.set(value);
        break;
    case 1:
        odometer_y.set(value);
        break;
    case 2:
        odometer_z.set(value);
        break;
    default:
        debug_assert(false && "invalid index");
        return;
    }
}

float CurrentStore::get_odometer_extruded_length(PhysicalToolIndex tool) {
    return odometer_extruded_lengths.get(tool.to_raw());
}

void CurrentStore::set_odometer_extruded_length(PhysicalToolIndex tool, float value) {
    odometer_extruded_lengths.set(tool.to_raw(), value);
}

uint32_t CurrentStore::get_odometer_toolpicks(PhysicalToolIndex tool) {
    return odometer_toolpicks.get(tool.to_raw());
}

void CurrentStore::set_odometer_toolpicks(PhysicalToolIndex tool, uint32_t value) {
    odometer_toolpicks.set(tool.to_raw(), value);
}

#if HAS_SHEET_PROFILES()
Sheet CurrentStore::get_sheet(uint8_t index) {
    debug_assert(index < config_store_ns::sheets_num);
    switch (index) {
    case 0:
        return sheet_0.get();
    case 1:
        return sheet_1.get();
    case 2:
        return sheet_2.get();
    case 3:
        return sheet_3.get();
    case 4:
        return sheet_4.get();
    case 5:
        return sheet_5.get();
    case 6:
        return sheet_6.get();
    case 7:
        return sheet_7.get();
    default:
        debug_assert(false && "invalid index");
        return {};
    }
}

void CurrentStore::set_sheet(uint8_t index, Sheet value) {
    debug_assert(index < config_store_ns::sheets_num);
    switch (index) {
    case 0:
        sheet_0.set(value);
        break;
    case 1:
        sheet_1.set(value);
        break;
    case 2:
        sheet_2.set(value);
        break;
    case 3:
        sheet_3.set(value);
        break;
    case 4:
        sheet_4.set(value);
        break;
    case 5:
        sheet_5.set(value);
        break;
    case 6:
        sheet_6.set(value);
        break;
    case 7:
        sheet_7.set(value);
        break;
    default:
        debug_assert(false && "invalid index");
        return;
    }
}
#endif

#if HAS_15GT_BELTS()
bool CurrentStore::set_belts_15gt(bool installed) {
    if (belts_15gt_installed.get() == installed) {
        return false;
    }
    auto transaction = get_backend().transaction_guard();
    belts_15gt_installed.set(installed);
    #if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    // Clear any manual override so the resolved default follows the belt HW.
    axis_steps_per_unit_x.set_to_default();
    axis_steps_per_unit_y.set_to_default();
    #endif
    // Belt type changes X/Y steps/mm -> XY geometry calibration and axis selftest are invalid.
    homing_sens_x.set_to_default();
    homing_sens_y.set_to_default();
    homing_bump_divisor_x.set_to_default();
    homing_bump_divisor_y.set_to_default();
    #if HAS_PRECISE_HOMING()
    precise_homing_sample_history.set_all_to_default();
    precise_homing_sample_history_index.set_all_to_default();
    #endif
    #if HAS_PRECISE_HOMING_COREXY()
    // The grid origin is a motor-phase-to-position mapping, so a different belt pitch invalidates it.
    corexy_grid_origin.set_to_default();
    precise_homing_instability_history.set_to_default();
        #if HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
    corexy_home_tmc_sens.set_to_default();
        #endif
    #endif
    #if HAS_MANUAL_BELT_TUNING()
    manual_belt_tuning_completed.set_to_default();
    #endif
    #if HAS_SELFTEST()
    selftest_result.apply([](SelftestResult &r) {
        r.set_xaxis(TestResult::unknown);
        r.set_yaxis(TestResult::unknown);
    });
    #endif
    return true;
}
#endif

input_shaper::Config CurrentStore::get_input_shaper_config() {
    input_shaper::Config config;
    if (input_shaper_axis_x_enabled.get()) {
        config.axis[X_AXIS] = input_shaper_axis_x_config.get();
    } else {
        config.axis[X_AXIS] = std::nullopt;
    }
    if (input_shaper_axis_y_enabled.get()) {
        config.axis[Y_AXIS] = input_shaper_axis_y_config.get();
    } else {
        config.axis[Y_AXIS] = std::nullopt;
    }
    if (input_shaper_weight_adjust_y_enabled.get()) {
        config.weight_adjust_y = input_shaper_weight_adjust_y_config.get();
    } else {
        config.weight_adjust_y = std::nullopt;
    }
    return config;
}

void CurrentStore::set_input_shaper_config(const input_shaper::Config &config) {
    if (config.axis[X_AXIS]) {
        input_shaper_axis_x_config.set(*config.axis[X_AXIS]);
        input_shaper_axis_x_enabled.set(true);
    } else {
        input_shaper_axis_x_enabled.set(false);
    }
    if (config.axis[Y_AXIS]) {
        input_shaper_axis_y_config.set(*config.axis[Y_AXIS]);
        input_shaper_axis_y_enabled.set(true);
    } else {
        input_shaper_axis_y_enabled.set(false);
    }
    if (config.weight_adjust_y) {
        input_shaper_weight_adjust_y_config.set(*config.weight_adjust_y);
        input_shaper_weight_adjust_y_enabled.set(true);
    } else {
        input_shaper_weight_adjust_y_enabled.set(false);
    }
}

input_shaper::AxisConfig CurrentStore::get_input_shaper_axis_config(AxisEnum axis) {
    switch (axis) {

    case X_AXIS:
        return input_shaper_axis_x_config.get();

    case Y_AXIS:
        return input_shaper_axis_y_config.get();

    default:
        std::abort();
    }
}

void CurrentStore::set_input_shaper_axis_config(AxisEnum axis, const input_shaper::AxisConfig &config) {
    switch (axis) {

    case X_AXIS:
        input_shaper_axis_x_config.set(config);
        break;

    case Y_AXIS:
        input_shaper_axis_y_config.set(config);
        break;

    default:
        std::abort();
    }
}

#if HAS_PHASE_STEPPING()
bool CurrentStore::get_phase_stepping_enabled() {
    return get_phase_stepping_enabled(AxisEnum::X_AXIS) || get_phase_stepping_enabled(AxisEnum::Y_AXIS);
}

bool CurrentStore::get_phase_stepping_enabled(AxisEnum axis) {
    switch (axis) {
    case AxisEnum::X_AXIS:
        return phase_stepping_enabled_x.get();
        break;
    case AxisEnum::Y_AXIS:
        return phase_stepping_enabled_y.get();
        break;
    default:
        debug_assert(false && "invalid index");
        return {};
    }
}

void CurrentStore::set_phase_stepping_enabled(AxisEnum axis, bool new_state) {
    switch (axis) {
    case AxisEnum::X_AXIS:
        phase_stepping_enabled_x.set(new_state);
        break;
    case AxisEnum::Y_AXIS:
        phase_stepping_enabled_y.set(new_state);
        break;
    default:
        debug_assert(false && "invalid index");
        return;
    }
}
#endif

#if HAS_AUTO_RETRACT()

void CurrentStore::set_filament_retracted_distance(PhysicalToolIndex tool, std::optional<float> dist) {
    if (!dist.has_value()) {
        filament_retracted_distances.set(tool.to_raw(), invalid_retracted_distance);
        return;
    }

    const float rounded_dist = std::round(dist.value());
    const float clamped_dist = std::clamp<float>(rounded_dist, 0, invalid_retracted_distance - 1);
    debug_assert(clamped_dist == rounded_dist);
    filament_retracted_distances.set(tool.to_raw(), static_cast<uint8_t>(clamped_dist));
}

std::optional<float> CurrentStore::get_filament_retracted_distance(PhysicalToolIndex tool) {
    const auto distance = filament_retracted_distances.get(tool.to_raw());
    if (distance == invalid_retracted_distance) {
        return std::nullopt;
    }
    return distance;
}

#endif

#if HAS_CHAMBER_VENTS()
VentControl CurrentStore::get_vent_control() {
    if (!check_chamber_vent_state.get()) {
        return VentControl::off;
    } else {
        return auto_chamber_vent_enabled.get() ? VentControl::automatic : VentControl::manual;
    }
}

void CurrentStore::set_vent_control(VentControl state) {
    switch (state) {
    case VentControl::off:
        check_chamber_vent_state.set(false);
        break;
    case VentControl::automatic:
        check_chamber_vent_state.set(true);
        auto_chamber_vent_enabled.set(true);
        break;
    case VentControl::manual:
        check_chamber_vent_state.set(true);
        auto_chamber_vent_enabled.set(false);
        break;
    }
}
#endif

#if HAS_INDX()
bool CurrentStore::is_indx_tool_enabled(PhysicalToolIndex tool) {
    return indx_enabled_tools.get().test(tool.to_raw());
}

void CurrentStore::set_indx_tool_enabled(PhysicalToolIndex tool, bool enabled) {
    auto mask = indx_enabled_tools.get();
    mask.set(tool.to_raw(), enabled);
    indx_enabled_tools.set(mask);
}

std::variant<PhysicalToolIndex, NoTool> CurrentStore::get_indx_last_picked_tool() {
    if (indx_last_picked_tool.get() == defaults::no_tool_value) {
        return NoTool {};
    }

    return PhysicalToolIndex::from_raw(indx_last_picked_tool.get());
}

void CurrentStore::set_indx_last_picked_tool(std::variant<PhysicalToolIndex, NoTool> tool) {
    if (std::holds_alternative<NoTool>(tool)) {
        indx_last_picked_tool.set(defaults::no_tool_value);
    } else {
        indx_last_picked_tool.set(std::get<PhysicalToolIndex>(tool).to_raw());
    }
}

#endif

} // namespace config_store_ns
