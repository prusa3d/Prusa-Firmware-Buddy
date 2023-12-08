#include "MItem_input_shaper.hpp"

#include "ScreenHandler.hpp"

static bool input_shaper_x_enabled() {
    return input_shaper::current_config().axis[X_AXIS].has_value();
}

static bool input_shaper_y_enabled() {
    return input_shaper::current_config().axis[Y_AXIS].has_value();
}

static int32_t input_shaper_x_type() {
    const auto axis_x = input_shaper::current_config().axis[X_AXIS];
    return static_cast<int32_t>(axis_x->type);
}

static int32_t input_shaper_y_type() {
    const auto axis_y = input_shaper::current_config().axis[Y_AXIS];
    return static_cast<int32_t>(axis_y->type);
}

static uint32_t input_shaper_x_frequency() {
    const auto axis_x = input_shaper::current_config().axis[X_AXIS];
    return static_cast<int32_t>(axis_x->frequency);
}

static uint32_t input_shaper_y_frequency() {
    const auto axis_y = input_shaper::current_config().axis[Y_AXIS];
    return static_cast<int32_t>(axis_y->frequency);
}

static bool input_shaper_y_weight_compensation() {
    return input_shaper::current_config().weight_adjust_y.has_value();
}

MI_IS_X_ONOFF::MI_IS_X_ONOFF()
    : WI_ICON_SWITCH_OFF_ON_t(input_shaper_x_enabled(), _(label), nullptr, is_enabled_t::no, is_hidden_t::yes) {
}

void MI_IS_X_ONOFF::OnChange(size_t) {
    config_store().input_shaper_axis_x_enabled.set(index);
    if (index) {
        input_shaper::set_axis_config(X_AXIS, config_store().input_shaper_axis_x_config.get());
    } else {
        input_shaper::set_axis_config(X_AXIS, std::nullopt);
    }
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)&param);
}

MI_IS_Y_ONOFF::MI_IS_Y_ONOFF()
    : WI_ICON_SWITCH_OFF_ON_t(input_shaper_y_enabled(), _(label), nullptr, is_enabled_t::no, is_hidden_t::yes) {
}

void MI_IS_Y_ONOFF::OnChange(size_t) {
    config_store().input_shaper_axis_y_enabled.set(index);
    if (index) {
        input_shaper::set_axis_config(Y_AXIS, config_store().input_shaper_axis_y_config.get());
    } else {
        input_shaper::set_axis_config(Y_AXIS, std::nullopt);
    }
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)&param);
}

MI_IS_X_TYPE::MI_IS_X_TYPE()
    // clang-format off
    : WI_SWITCH_t<6>(input_shaper_x_type(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zvd))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::mzv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_2hump))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_3hump))
    ) {
    // clang-format on
    if (!input_shaper_x_enabled()) {
        DontShowDisabledExtension();
    }
}

void MI_IS_X_TYPE::OnChange(size_t) {
    auto axis_x = config_store().input_shaper_axis_x_config.get();
    axis_x.type = static_cast<input_shaper::Type>(GetIndex());
    config_store().input_shaper_axis_x_config.set(axis_x);
    input_shaper::set_axis_config(X_AXIS, axis_x);
}

MI_IS_Y_TYPE::MI_IS_Y_TYPE()
    // clang-format off
    : WI_SWITCH_t<6>(input_shaper_y_type(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zvd))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::mzv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_2hump))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_3hump))
    ) {
    // clang-format on
    if (!input_shaper_y_enabled()) {
        DontShowDisabledExtension();
    }
}

void MI_IS_Y_TYPE::OnChange(size_t) {
    auto axis_y = config_store().input_shaper_axis_y_config.get();
    axis_y.type = static_cast<input_shaper::Type>(GetIndex());
    config_store().input_shaper_axis_y_config.set(axis_y);
    input_shaper::set_axis_config(Y_AXIS, axis_y);
}

static constexpr SpinConfigInt is_frequency_spin_config = makeSpinConfig<int>(
    { static_cast<int>(input_shaper::frequency_safe_min), static_cast<int>(input_shaper::frequency_safe_max), 1 },
    "Hz",
    spin_off_opt_t::no);

MI_IS_X_FREQUENCY::MI_IS_X_FREQUENCY()
    : WiSpinInt(input_shaper_x_frequency(), is_frequency_spin_config, _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
    if (!input_shaper_x_enabled()) {
        DontShowDisabledExtension();
    }
}

void MI_IS_X_FREQUENCY::OnClick() {
    auto axis_x = config_store().input_shaper_axis_x_config.get();
    axis_x.frequency = static_cast<float>(GetVal());
    config_store().input_shaper_axis_x_config.set(axis_x);
    input_shaper::set_axis_config(X_AXIS, axis_x);
}

MI_IS_Y_FREQUENCY::MI_IS_Y_FREQUENCY()
    : WiSpinInt(input_shaper_y_frequency(), is_frequency_spin_config, _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
    if (!input_shaper_y_enabled()) {
        DontShowDisabledExtension();
    }
}

void MI_IS_Y_FREQUENCY::OnClick() {
    auto axis_y = config_store().input_shaper_axis_y_config.get();
    axis_y.frequency = static_cast<float>(GetVal());
    config_store().input_shaper_axis_y_config.set(axis_y);
    input_shaper::set_axis_config(Y_AXIS, axis_y);
}

MI_IS_Y_COMPENSATION::MI_IS_Y_COMPENSATION()
    : WI_ICON_SWITCH_OFF_ON_t(input_shaper_y_weight_compensation(), _(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {
    if (!input_shaper_y_enabled()) {
        DontShowDisabledExtension();
    }
}

void MI_IS_Y_COMPENSATION::OnChange(size_t) {
    config_store().input_shaper_weight_adjust_y_enabled.set(index);
    if (index) {
        input_shaper::current_config().weight_adjust_y = config_store().input_shaper_weight_adjust_y_config.get();
    } else {
        input_shaper::current_config().weight_adjust_y = std::nullopt;
    }
}

MI_IS_SET::MI_IS_SET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_IS_SET::click(IWindowMenu &) {
    MsgBoxISWarning(_("ATTENTION: Changing any Input Shaper values will overwrite them permanently. To revert to a stock setup, visit prusa.io/input-shaper or run a factory reset."), Responses_Ok);
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)&param);
}

MI_IS_CALIB::MI_IS_CALIB()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_IS_CALIB::click([[maybe_unused]] IWindowMenu &window_menu) {
    // TODO(InputShaper)
}
