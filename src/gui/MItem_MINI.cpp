/**
 * @file MItem_MINI.cpp
 */

#include "MItem_MINI.hpp"
#include "img_resources.hpp"
#include "filament_sensors_handler.hpp"
#include "filament_sensor.hpp"
#include "str_utils.hpp"
#include "fonts.hpp"

static string_view_utf8 to_string_view_utf8(FilamentSensorState filament_sensor_state) {
    switch (filament_sensor_state) {
    case FilamentSensorState::HasFilament:
        return string_view_utf8::MakeCPUFLASH("1"); // Intentionally not translated
    case FilamentSensorState::NoFilament:
        return string_view_utf8::MakeCPUFLASH("0"); // Intentionally not translated
    case FilamentSensorState::Disabled:
        return _("Disabled");
    case FilamentSensorState::NotInitialized:
    case FilamentSensorState::NotCalibrated:
    case FilamentSensorState::NotConnected:
        return string_view_utf8::MakeCPUFLASH("N/A"); // Intentionally not translated
    }
    abort();
}

void MI_FILAMENT_SENSOR_STATE::printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn) const {
    render_text_align(extension_rect, to_string_view_utf8(state), GuiDefaults::FontMenuItems, color_back, color_text, padding_ui8(0, 4, 0, 0), Align_t::Center(), false);
}

MI_FILAMENT_SENSOR_STATE::MI_FILAMENT_SENSOR_STATE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
    Loop();
}

void MI_FILAMENT_SENSOR_STATE::Loop() {
    const FilamentSensorState new_state = FSensors_instance().sensor_state(LogicalFilamentSensor::primary_runout);
    if (new_state == state) {
        return;
    }
    state = new_state;
    string_view_utf8 current_item_text = to_string_view_utf8(state);

    const Rect16::Width_t new_extension_width = width(GuiDefaults::FontMenuItems) * current_item_text.computeNumUtf8Chars() + GuiDefaults::MenuPaddingItems.left + GuiDefaults::MenuPaddingItems.right;
    if (extension_width != new_extension_width) {
        extension_width = new_extension_width;
        Invalidate();
    } else {
        InValidateExtension();
    }
}

MI_MINDA::MI_MINDA()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_MINDA::state_t MI_MINDA::get_state() {
    return (buddy::hw::zMin.read() == buddy::hw::Pin::State::low) ? state_t::low : state_t::high;
}

void MI_MINDA::Loop() {
    SetIndex((size_t)get_state());
}
