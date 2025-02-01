#include "screen_menu_filament_sensors.hpp"

#include "filament_sensors_handler.hpp"
#include "ScreenHandler.hpp"

using namespace NScreenMenuFilamentSensors;

IMI_AnySensor::IMI_AnySensor(uint8_t sensor_index, bool is_side, const char *label_base)
    : WI_ICON_SWITCH_OFF_ON_t(0, {}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , sensor_index(sensor_index)
    , is_side(is_side) //
{
    update();

    // Format label
    {
        StringBuilder sb(label);
        sb.append_string_view(_(label_base));

        if constexpr (toolhead_count > 1) {
            sb.append_printf(" %i", sensor_index + 1);
        }

        SetLabel(string_view_utf8::MakeRAM(label.data()));
    }
}

void IMI_AnySensor::update() {
    // Disable the item if the global filament enable is off
    set_enabled(config_store().fsensor_enabled.get());

#if HAS_TOOLCHANGER()
    set_is_hidden(!prusa_toolchanger.is_tool_enabled(sensor_index));
#endif

    const uint8_t bit_mask = (1 << sensor_index);

    if (is_side) {
        set_index(bool(config_store().fsensor_side_enabled_bits.get() & bit_mask));
    } else {
        set_index(bool(config_store().fsensor_extruder_enabled_bits.get() & bit_mask));
    }
}

void IMI_AnySensor::OnChange(size_t old_index) {
    // Enabling/disabling FS can generate gcodes (I'm looking at you, MMU!).
    // Fail the action if there's no space in the queue.
    if (!gui_check_space_in_gcode_queue_with_msg()) {
        // set_index doesn't call OnChange
        set_index(old_index);
        return;
    }

    const uint8_t bit_mask = (1 << sensor_index);
    const auto update_f = [&](auto val) { return index ? (val | bit_mask) : (val & ~bit_mask); };

    if (is_side) {
        config_store().fsensor_side_enabled_bits.transform(update_f);
    } else {
        config_store().fsensor_extruder_enabled_bits.transform(update_f);
    }

    auto &fss = FSensors_instance();
    fss.request_enable_state_update();

    if (index && !fss.gui_wait_for_init_with_msg()) {
        set_index(0);
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

    // Signal to the parent screen to update other items
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, nullptr);
}

ScreenMenuFilamentSensors::ScreenMenuFilamentSensors()
    : MenuBase(_(label)) {
}

void ScreenMenuFilamentSensors::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CHILD_CLICK:
        [&]<size_t... i>(std::index_sequence<i...>) {
            Item<MI_FILAMENT_SENSOR>().update();
            (Item<MI_ExtruderSensor<i>>().update(), ...);
#if HAS_SIDE_FSENSOR()
            (Item<MI_SideSensor<i>>().update(), ...);
#endif
        }(std::make_index_sequence<toolhead_count>());
        break;

    default:
        NScreenMenuFilamentSensors::MenuBase::windowEvent(sender, event, param);
        break;
    }
}
