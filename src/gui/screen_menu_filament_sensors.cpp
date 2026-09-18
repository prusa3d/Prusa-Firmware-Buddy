#include "screen_menu_filament_sensors.hpp"

#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <window_msgbox.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

using namespace NScreenMenuFilamentSensors;

namespace {

constexpr TranslatableString extruder_sensor_label = //
    PhysicalToolIndex::count > 1 ? (TranslatableString)N_("Toolhead Filament Sensor %i")
                                 : (TranslatableString)N_("Toolhead Filament Sensor");

constexpr TranslatableString side_sensor_label = //
    PhysicalToolIndex::count > 1 ? (TranslatableString)N_("Side Filament Sensor %i")
                                 : (TranslatableString)N_("Side Filament Sensor");

} // namespace

IMI_AnySensor::IMI_AnySensor(uint8_t sensor_index, bool is_side)
    : WI_ICON_SWITCH_OFF_ON_t(0, {}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , sensor_index(sensor_index)
    , is_side(is_side) //
{
    update();

    const auto tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::from_raw_notool(sensor_index));
    set_is_hidden(!(tool.has_value() && tool->is_enabled()));

    const TranslatableString label = is_side ? side_sensor_label : extruder_sensor_label;
    if constexpr (PhysicalToolIndex::count > 1) {
        SetLabel(string_view_utf8(label).formatted(label_params_, sensor_index + 1));
    } else {
        SetLabel(label);
    }
}

void IMI_AnySensor::update() {
    // Disable the item if the global filament enable is off
    set_enabled(config_store().fsensor_enabled.get());

    const uint8_t bit_mask = (1 << sensor_index);

    if (is_side) {
        set_value(bool(config_store().fsensor_side_enabled_bits.get() & bit_mask));
    } else {
        set_value(bool(config_store().fsensor_extruder_enabled_bits.get() & bit_mask));
    }
}

void IMI_AnySensor::Loop() {
    update();
}

void IMI_AnySensor::OnChange(size_t old_index) {
    // Enabling/disabling FS can generate gcodes (I'm looking at you, MMU!).
    // Fail the action if there's no space in the queue.
    if (!gui_check_space_in_gcode_queue_with_msg()) {
        set_value(old_index > 0);
        return;
    }

    const uint8_t bit_mask = (1 << sensor_index);
    const auto update_f = [&](auto val) { return value() ? (val | bit_mask) : (val & ~bit_mask); };

    if (is_side) {
        config_store().fsensor_side_enabled_bits.transform(update_f);
    } else {
        config_store().fsensor_extruder_enabled_bits.transform(update_f);
    }

    auto &fss = FSensors_instance();
    fss.request_enable_state_update();

    if (value() && !fss.gui_wait_for_init_with_msg()) {
        set_value(false);
    }
}

MI_RestoreDefaults::MI_RestoreDefaults()
    : IWindowMenuItem(_(label)) {}

void MI_RestoreDefaults::click(IWindowMenu &) {
    // Enabling/disabling FS can generate gcodes (I'm looking at you, MMU!).
    // Fail the action if there's no space in the queue.
    if (!gui_check_space_in_gcode_queue_with_msg()) {
        return;
    }

    config_store().fsensor_enabled.set(true);
    config_store().fsensor_side_enabled_bits.set_to_default();
    config_store().fsensor_extruder_enabled_bits.set_to_default();

    auto &fss = FSensors_instance();
    fss.request_enable_state_update();

    if (!fss.gui_wait_for_init_with_msg()) {
        fss.set_enabled_global(false);
    } else {
        MsgBoxInfo(_("All filament sensors enabled."), Responses_Ok);
    }
}

ScreenMenuFilamentSensors::ScreenMenuFilamentSensors()
    : MenuBase(_(label)) {
}
