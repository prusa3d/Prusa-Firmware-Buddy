#include "MItem_input_shaper.hpp"

#include "ScreenHandler.hpp"

MI_IS_X_ONOFF::MI_IS_X_ONOFF()
    : WI_ICON_SWITCH_OFF_ON_t(false /* set in ScreenMenuInputShaper::update_gui*/, _(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {
    static_assert(decltype(config_store_ns::CurrentStore::input_shaper_axis_x_enabled)::default_val, "If input shaper is not enabled by default, please unhide this item");
}

void MI_IS_X_ONOFF::OnChange(size_t) {
    config_store().input_shaper_axis_x_enabled.set(index);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");

    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, ftrstd::bit_cast<void *>(InputShaperMenuItemChildClickParam::request_gui_update));
}

MI_IS_Y_ONOFF::MI_IS_Y_ONOFF()
    : WI_ICON_SWITCH_OFF_ON_t(false /* set in ScreenMenuInputShaper::update_gui*/, _(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {
    static_assert(decltype(config_store_ns::CurrentStore::input_shaper_axis_y_enabled)::default_val, "If input shaper is not enabled by default, please unhide this item");
}

void MI_IS_Y_ONOFF::OnChange(size_t) {
    config_store().input_shaper_axis_y_enabled.set(index);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");

    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, ftrstd::bit_cast<void *>(InputShaperMenuItemChildClickParam::request_gui_update));
}

MI_IS_X_TYPE::MI_IS_X_TYPE()
    : WiEnumSwitch(/* index is set in ScreenMenuInputShaper::update_gui*/ _(label), input_shaper::filter_names, false, input_shaper::enabled_filters) {
}

void MI_IS_X_TYPE::OnChange(size_t) {
    auto axis_x = config_store().input_shaper_axis_x_config.get();
    axis_x.type = static_cast<input_shaper::Type>(GetIndex());
    config_store().input_shaper_axis_x_config.set(axis_x);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");
}

MI_IS_Y_TYPE::MI_IS_Y_TYPE()
    : WiEnumSwitch(/* set in ScreenMenuInputShaper::update_gui*/ _(label), input_shaper::filter_names, false, input_shaper::enabled_filters) {
}

void MI_IS_Y_TYPE::OnChange(size_t) {
    auto axis_y = config_store().input_shaper_axis_y_config.get();
    axis_y.type = static_cast<input_shaper::Type>(GetIndex());
    config_store().input_shaper_axis_y_config.set(axis_y);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");
}

static constexpr NumericInputConfig is_frequency_spin_config {
    .min_value = input_shaper::frequency_safe_min,
    .max_value = input_shaper::frequency_safe_max,
    .unit = Unit::hertz,
};

MI_IS_X_FREQUENCY::MI_IS_X_FREQUENCY()
    : WiSpin(0 /* set in ScreenMenuInputShaper::update_gui*/, is_frequency_spin_config, _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_IS_X_FREQUENCY::OnClick() {
    auto axis_x = config_store().input_shaper_axis_x_config.get();
    axis_x.frequency = static_cast<float>(GetVal());
    config_store().input_shaper_axis_x_config.set(axis_x);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");
}

MI_IS_Y_FREQUENCY::MI_IS_Y_FREQUENCY()
    : WiSpin(0 /* set in ScreenMenuInputShaper::update_gui*/, is_frequency_spin_config, _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_IS_Y_FREQUENCY::OnClick() {
    auto axis_y = config_store().input_shaper_axis_y_config.get();
    axis_y.frequency = static_cast<float>(GetVal());
    config_store().input_shaper_axis_y_config.set(axis_y);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");
}

MI_IS_Y_COMPENSATION::MI_IS_Y_COMPENSATION()
    : WI_ICON_SWITCH_OFF_ON_t(false /* set in ScreenMenuInputShaper::update_gui*/, _(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {
}

void MI_IS_Y_COMPENSATION::OnChange(size_t) {
    config_store().input_shaper_weight_adjust_y_enabled.set(index);

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");
}

MI_IS_ENABLE_EDITING::MI_IS_ENABLE_EDITING()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::yes) {
}

void MI_IS_ENABLE_EDITING::click(IWindowMenu &) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, ftrstd::bit_cast<void *>(InputShaperMenuItemChildClickParam::enable_editing));
}

#if HAS_INPUT_SHAPER_CALIBRATION()
MI_IS_CALIB::MI_IS_CALIB()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, marlin_client::is_printing() ? is_hidden_t::yes : is_hidden_t::no) {
}

void MI_IS_CALIB::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M1959");
}
#endif

MI_IS_RESTORE_DEFAULTS::MI_IS_RESTORE_DEFAULTS()
    : IWindowMenuItem(_(label), nullptr) {
}

void MI_IS_RESTORE_DEFAULTS::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (MsgBoxQuestion(_("Do you really want to restore default input shaper configuration?"), Responses_YesNo) != Response::Yes) {
        return;
    }

    // Restore defaults in the config store
    {
        auto &store = config_store();
        auto transaction = store.get_backend().transaction_guard();
        store.input_shaper_axis_x_config.set_to_default();
        store.input_shaper_axis_y_config.set_to_default();
        store.input_shaper_weight_adjust_y_config.set_to_default();
        store.input_shaper_axis_x_enabled.set_to_default();
        store.input_shaper_axis_y_enabled.set_to_default();
        store.input_shaper_weight_adjust_y_enabled.set_to_default();
    }

    // Make the input shaper reload config from config_store
    gui_try_gcode_with_msg("M9200");

    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, ftrstd::bit_cast<void *>(InputShaperMenuItemChildClickParam::request_gui_update));
}
