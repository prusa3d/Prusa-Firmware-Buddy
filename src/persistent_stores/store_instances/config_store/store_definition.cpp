#include "store_definition.hpp"
#include <Marlin/src/inc/MarlinConfigPre.h>
#include <module/prusa/dock_position.hpp>
#include <module/prusa/tool_offset.hpp>
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#include <option/has_config_store_wo_backend.h>
#include <option/has_touch.h>
#include <sys.h>

namespace config_store_ns {
#if not HAS_CONFIG_STORE_WO_BACKEND()
static_assert((sizeof(CurrentStore) + aggregate_arity<CurrentStore>() * sizeof(journal::Backend::ItemHeader)) < (BANK_SIZE / 100) * 75, "EEPROM bank is almost full");
static_assert(journal::has_unique_items<config_store_ns::CurrentStore>(), "Just added items are causing collisions with reserved backend IDs");
#endif

void CurrentStore::perform_config_check() {
    /// Whether this is the first run of the printer after assembly/factory reset
    [[maybe_unused]] const bool is_first_run = (config_store_init_result() == InitResult::cold_start);

    // Do not show pritner setup screen if the user has run any selftests
    // This is for backwards compatibility - we don't want to show the screen after the firmware update introducing it for already configured printers
    if (selftest_result.get() != selftest_result.default_val) {
        printer_setup_done.set(true);
    }

    // We cannot change a default value of config store items for backwards compatibility reasons.
    // So this is a place to instead set them to something for new installations
    if (is_first_run) {
#if HAS_TOUCH()
        touch_enabled.set(true);
#endif

#if PRINTER_IS_PRUSA_MK4()
        static_assert(extended_printer_type_model[1] == PrinterModel::mk4s);
        extended_printer_type.set(1);
        hotend_type.set(HotendType::stock_with_sock);
        nozzle_is_high_flow.set(1 << 0); // Bitset -> first and only nozzle

#elif PRINTER_IS_PRUSA_XL()
        // New XL printers have .4mm nozzles: BFW-5638
        for (int i = 0; i < HOTENDS; i++) {
            set_nozzle_diameter(i, 0.4f);
        }

#elif PRINTER_IS_PRUSA_MK3_5()
        static_assert(extended_printer_type_model[1] == PrinterModel::mk3_5s);
        extended_printer_type.set(1);

#endif
    }

    // BFW-5486
    // Auto-update is now enablablable only in develeoper mode
    // There were some issues with people leaving this option on, then upgrading and having problems turning it off
    if constexpr (!option::development_items) {
        sys_fw_update_disable();
    }

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

footer::Item CurrentStore::get_footer_setting([[maybe_unused]] uint8_t index) {
    switch (index) {
    case 0:
        return footer_setting_0.get();
#if FOOTER_ITEMS_PER_LINE__ > 1
    case 1:
        return footer_setting_1.get();
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    case 2:
        return footer_setting_2.get();
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    case 3:
        return footer_setting_3.get();
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    case 4:
        return footer_setting_4.get();
#endif
    default:
        assert(false && "invalid index");
        return footer::Item::none;
    }
}

void CurrentStore::set_footer_setting(uint8_t index, footer::Item value) {
    switch (index) {
    case 0:
        footer_setting_0.set(value);
        break;
#if FOOTER_ITEMS_PER_LINE__ > 1
    case 1:
        footer_setting_1.set(value);
        break;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    case 2:
        footer_setting_2.set(value);
        break;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    case 3:
        footer_setting_3.set(value);
        break;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    case 4:
        footer_setting_4.set(value);
        break;
#endif
    default:
        assert(false && "invalid index");
        return;
    }
}

int32_t CurrentStore::get_extruder_fs_ref_nins_value([[maybe_unused]] uint8_t index) {
#if HOTENDS <= 1
    assert(index == 0);
    return extruder_fs_ref_nins_value_0.get();
#else
    switch (index) {
    case 0:
        return extruder_fs_ref_nins_value_0.get();
    case 1:
        return extruder_fs_ref_nins_value_1.get();
    case 2:
        return extruder_fs_ref_nins_value_2.get();
    case 3:
        return extruder_fs_ref_nins_value_3.get();
    case 4:
        return extruder_fs_ref_nins_value_4.get();
    case 5:
        return extruder_fs_ref_nins_value_5.get();
    default:
        assert(false && "invalid index");
        return 0;
    }
#endif
}

void CurrentStore::set_extruder_fs_ref_nins_value([[maybe_unused]] uint8_t index, int32_t value) {
#if HOTENDS <= 1
    assert(index == 0);
    extruder_fs_ref_nins_value_0.set(value);
#else
    switch (index) {
    case 0:
        extruder_fs_ref_nins_value_0.set(value);
        break;
    case 1:
        extruder_fs_ref_nins_value_1.set(value);
        break;
    case 2:
        extruder_fs_ref_nins_value_2.set(value);
        break;
    case 3:
        extruder_fs_ref_nins_value_3.set(value);
        break;
    case 4:
        extruder_fs_ref_nins_value_4.set(value);
        break;
    case 5:
        extruder_fs_ref_nins_value_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
#endif
}

int32_t CurrentStore::get_extruder_fs_ref_ins_value([[maybe_unused]] uint8_t index) {
#if HOTENDS <= 1
    assert(index == 0);
    return extruder_fs_ref_ins_value_0.get();
#else
    switch (index) {
    case 0:
        return extruder_fs_ref_ins_value_0.get();
    case 1:
        return extruder_fs_ref_ins_value_1.get();
    case 2:
        return extruder_fs_ref_ins_value_2.get();
    case 3:
        return extruder_fs_ref_ins_value_3.get();
    case 4:
        return extruder_fs_ref_ins_value_4.get();
    case 5:
        return extruder_fs_ref_ins_value_5.get();
    default:
        assert(false && "invalid index");
        return 0;
    }
#endif
}

void CurrentStore::set_extruder_fs_ref_ins_value([[maybe_unused]] uint8_t index, int32_t value) {
#if HOTENDS <= 1
    assert(index == 0);
    extruder_fs_ref_ins_value_0.set(value);
#else
    switch (index) {
    case 0:
        extruder_fs_ref_ins_value_0.set(value);
        break;
    case 1:
        extruder_fs_ref_ins_value_1.set(value);
        break;
    case 2:
        extruder_fs_ref_ins_value_2.set(value);
        break;
    case 3:
        extruder_fs_ref_ins_value_3.set(value);
        break;
    case 4:
        extruder_fs_ref_ins_value_4.set(value);
        break;
    case 5:
        extruder_fs_ref_ins_value_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
#endif
}

uint32_t CurrentStore::get_extruder_fs_value_span([[maybe_unused]] uint8_t index) {
#if HOTENDS <= 1
    assert(index == 0);
    return extruder_fs_value_span_0.get();
#else
    switch (index) {
    case 0:
        return extruder_fs_value_span_0.get();
    case 1:
        return extruder_fs_value_span_1.get();
    case 2:
        return extruder_fs_value_span_2.get();
    case 3:
        return extruder_fs_value_span_3.get();
    case 4:
        return extruder_fs_value_span_4.get();
    case 5:
        return extruder_fs_value_span_5.get();
    default:
        assert(false && "invalid index");
        return 0;
    }
#endif
}

void CurrentStore::set_extruder_fs_value_span([[maybe_unused]] uint8_t index, uint32_t value) {
#if HOTENDS <= 1
    assert(index == 0);
    extruder_fs_value_span_0.set(value);
#else
    switch (index) {
    case 0:
        extruder_fs_value_span_0.set(value);
        break;
    case 1:
        extruder_fs_value_span_1.set(value);
        break;
    case 2:
        extruder_fs_value_span_2.set(value);
        break;
    case 3:
        extruder_fs_value_span_3.set(value);
        break;
    case 4:
        extruder_fs_value_span_4.set(value);
        break;
    case 5:
        extruder_fs_value_span_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
#endif
}

#if HAS_SIDE_FSENSOR()
int32_t CurrentStore::get_side_fs_ref_nins_value(uint8_t index) {
    switch (index) {
    case 0:
        return side_fs_ref_nins_value_0.get();
    case 1:
        return side_fs_ref_nins_value_1.get();
    case 2:
        return side_fs_ref_nins_value_2.get();
    case 3:
        return side_fs_ref_nins_value_3.get();
    case 4:
        return side_fs_ref_nins_value_4.get();
    case 5:
        return side_fs_ref_nins_value_5.get();
    default:
        assert(false && "invalid index");
        return 0;
    }
}

void CurrentStore::set_side_fs_ref_nins_value(uint8_t index, int32_t value) {
    switch (index) {
    case 0:
        side_fs_ref_nins_value_0.set(value);
        break;
    case 1:
        side_fs_ref_nins_value_1.set(value);
        break;
    case 2:
        side_fs_ref_nins_value_2.set(value);
        break;
    case 3:
        side_fs_ref_nins_value_3.set(value);
        break;
    case 4:
        side_fs_ref_nins_value_4.set(value);
        break;
    case 5:
        side_fs_ref_nins_value_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
}

int32_t CurrentStore::get_side_fs_ref_ins_value(uint8_t index) {
    switch (index) {
    case 0:
        return side_fs_ref_ins_value_0.get();
    case 1:
        return side_fs_ref_ins_value_1.get();
    case 2:
        return side_fs_ref_ins_value_2.get();
    case 3:
        return side_fs_ref_ins_value_3.get();
    case 4:
        return side_fs_ref_ins_value_4.get();
    case 5:
        return side_fs_ref_ins_value_5.get();
    default:
        assert(false && "invalid index");
        return 0;
    }
}

void CurrentStore::set_side_fs_ref_ins_value(uint8_t index, int32_t value) {
    switch (index) {
    case 0:
        side_fs_ref_ins_value_0.set(value);
        break;
    case 1:
        side_fs_ref_ins_value_1.set(value);
        break;
    case 2:
        side_fs_ref_ins_value_2.set(value);
        break;
    case 3:
        side_fs_ref_ins_value_3.set(value);
        break;
    case 4:
        side_fs_ref_ins_value_4.set(value);
        break;
    case 5:
        side_fs_ref_ins_value_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
}

uint32_t CurrentStore::get_side_fs_value_span(uint8_t index) {
    switch (index) {
    case 0:
        return side_fs_value_span_0.get();
    case 1:
        return side_fs_value_span_1.get();
    case 2:
        return side_fs_value_span_2.get();
    case 3:
        return side_fs_value_span_3.get();
    case 4:
        return side_fs_value_span_4.get();
    case 5:
        return side_fs_value_span_5.get();
    default:
        assert(false && "invalid index");
        return 0;
    }
}

void CurrentStore::set_side_fs_value_span(uint8_t index, uint32_t value) {
    switch (index) {
    case 0:
        side_fs_value_span_0.set(value);
        break;
    case 1:
        side_fs_value_span_1.set(value);
        break;
    case 2:
        side_fs_value_span_2.set(value);
        break;
    case 3:
        side_fs_value_span_3.set(value);
        break;
    case 4:
        side_fs_value_span_4.set(value);
        break;
    case 5:
        side_fs_value_span_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
}
#endif

#if HAS_TOOLCHANGER()
DockPosition CurrentStore::get_dock_position(uint8_t index) {
    switch (index) {
    case 0:
        return dock_position_0.get();
    case 1:
        return dock_position_1.get();
    case 2:
        return dock_position_2.get();
    case 3:
        return dock_position_3.get();
    case 4:
        return dock_position_4.get();
    case 5:
        return dock_position_5.get();
    default:
        assert(false && "invalid index");
        return {};
    }
}

void CurrentStore::set_dock_position(uint8_t index, DockPosition value) {
    switch (index) {
    case 0:
        dock_position_0.set(value);
        break;
    case 1:
        dock_position_1.set(value);
        break;
    case 2:
        dock_position_2.set(value);
        break;
    case 3:
        dock_position_3.set(value);
        break;
    case 4:
        dock_position_4.set(value);
        break;
    case 5:
        dock_position_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
}

ToolOffset CurrentStore::get_tool_offset(uint8_t index) {
    switch (index) {
    case 0:
        return tool_offset_0.get();
    case 1:
        return tool_offset_1.get();
    case 2:
        return tool_offset_2.get();
    case 3:
        return tool_offset_3.get();
    case 4:
        return tool_offset_4.get();
    case 5:
        return tool_offset_5.get();
    default:
        assert(false && "invalid index");
        return {};
    }
}

void CurrentStore::set_tool_offset(uint8_t index, ToolOffset value) {
    switch (index) {
    case 0:
        tool_offset_0.set(value);
        break;
    case 1:
        tool_offset_1.set(value);
        break;
    case 2:
        tool_offset_2.set(value);
        break;
    case 3:
        tool_offset_3.set(value);
        break;
    case 4:
        tool_offset_4.set(value);
        break;
    case 5:
        tool_offset_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
}
#endif

FilamentType CurrentStore::get_filament_type([[maybe_unused]] uint8_t index) {
    return loaded_filament_type.get(index);
}

void CurrentStore::set_filament_type(uint8_t index, FilamentType value) {
    loaded_filament_type.set(index, value);
}

float CurrentStore::get_nozzle_diameter([[maybe_unused]] uint8_t index) {
#if HOTENDS <= 1
    assert(index == 0);
    return nozzle_diameter_0.get();
#else
    switch (index) {
    case 0:
        return nozzle_diameter_0.get();
    case 1:
        return nozzle_diameter_1.get();
    case 2:
        return nozzle_diameter_2.get();
    case 3:
        return nozzle_diameter_3.get();
    case 4:
        return nozzle_diameter_4.get();
    case 5:
        return nozzle_diameter_5.get();
    default:
        assert(false && "invalid index");
        return {};
    }
#endif
}

void CurrentStore::set_nozzle_diameter([[maybe_unused]] uint8_t index, float value) {
#if HOTENDS <= 1
    assert(index == 0);
    nozzle_diameter_0.set(value);
#else
    switch (index) {
    case 0:
        nozzle_diameter_0.set(value);
        break;
    case 1:
        nozzle_diameter_1.set(value);
        break;
    case 2:
        nozzle_diameter_2.set(value);
        break;
    case 3:
        nozzle_diameter_3.set(value);
        break;
    case 4:
        nozzle_diameter_4.set(value);
        break;
    case 5:
        nozzle_diameter_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
#endif
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
        assert(false && "invalid index");
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
        assert(false && "invalid index");
        return;
    }
}

float CurrentStore::get_odometer_extruded_length([[maybe_unused]] uint8_t index) {
#if HOTENDS <= 1
    assert(index == 0);
    return odometer_extruded_length_0.get();
#else
    switch (index) {
    case 0:
        return odometer_extruded_length_0.get();
    case 1:
        return odometer_extruded_length_1.get();
    case 2:
        return odometer_extruded_length_2.get();
    case 3:
        return odometer_extruded_length_3.get();
    case 4:
        return odometer_extruded_length_4.get();
    case 5:
        return odometer_extruded_length_5.get();
    default:
        assert(false && "invalid index");
        return {};
    }
#endif
}

void CurrentStore::set_odometer_extruded_length([[maybe_unused]] uint8_t index, float value) {
#if HOTENDS <= 1
    assert(index == 0);
    odometer_extruded_length_0.set(value);
#else
    switch (index) {
    case 0:
        odometer_extruded_length_0.set(value);
        break;
    case 1:
        odometer_extruded_length_1.set(value);
        break;
    case 2:
        odometer_extruded_length_2.set(value);
        break;
    case 3:
        odometer_extruded_length_3.set(value);
        break;
    case 4:
        odometer_extruded_length_4.set(value);
        break;
    case 5:
        odometer_extruded_length_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
#endif
}

uint32_t CurrentStore::get_odometer_toolpicks([[maybe_unused]] uint8_t index) {
#if HOTENDS <= 1
    assert(index == 0);
    return odometer_toolpicks_0.get();
#else
    switch (index) {
    case 0:
        return odometer_toolpicks_0.get();
    case 1:
        return odometer_toolpicks_1.get();
    case 2:
        return odometer_toolpicks_2.get();
    case 3:
        return odometer_toolpicks_3.get();
    case 4:
        return odometer_toolpicks_4.get();
    case 5:
        return odometer_toolpicks_5.get();
    default:
        assert(false && "invalid index");
        return {};
    }
#endif
}

void CurrentStore::set_odometer_toolpicks([[maybe_unused]] uint8_t index, uint32_t value) {
#if HOTENDS <= 1
    assert(index == 0);
    odometer_toolpicks_0.set(value);
#else
    switch (index) {
    case 0:
        odometer_toolpicks_0.set(value);
        break;
    case 1:
        odometer_toolpicks_1.set(value);
        break;
    case 2:
        odometer_toolpicks_2.set(value);
        break;
    case 3:
        odometer_toolpicks_3.set(value);
        break;
    case 4:
        odometer_toolpicks_4.set(value);
        break;
    case 5:
        odometer_toolpicks_5.set(value);
        break;
    default:
        assert(false && "invalid index");
        return;
    }
#endif
}
#if HAS_SELFTEST()
SelftestTool CurrentStore::get_selftest_result_tool(uint8_t index) {
    assert(index < config_store_ns::max_tool_count);
    return selftest_result.get().tools[index];
}

void CurrentStore::set_selftest_result_tool(uint8_t index, SelftestTool value) {
    assert(index < config_store_ns::max_tool_count);
    auto tmp = selftest_result.get();
    tmp.tools[index] = value;
    selftest_result.set(tmp);
}
#endif

#if HAS_SHEET_PROFILES()
Sheet CurrentStore::get_sheet(uint8_t index) {
    assert(index < config_store_ns::sheets_num);
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
        assert(false && "invalid index");
        return {};
    }
}

void CurrentStore::set_sheet(uint8_t index, Sheet value) {
    assert(index < config_store_ns::sheets_num);
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
        assert(false && "invalid index");
        return;
    }
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

#if HAS_PHASE_STEPPING()
bool CurrentStore::get_phase_stepping_enabled(AxisEnum axis) {
    switch (axis) {
    case AxisEnum::X_AXIS:
        return phase_stepping_enabled_x.get();
        break;
    case AxisEnum::Y_AXIS:
        return phase_stepping_enabled_y.get();
        break;
    default:
        assert(false && "invalid index");
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
        assert(false && "invalid index");
        return;
    }
}
#endif
} // namespace config_store_ns
