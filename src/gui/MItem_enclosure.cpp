
#include "MItem_enclosure.hpp"
#include "img_resources.hpp"
#include "WindowMenuSpin.hpp"
#include "xl_enclosure.hpp"
#include "ScreenHandler.hpp"
#include "screen_change_filter.hpp"
#include <str_utils.hpp>
#include <screen_menu_enclosure.hpp>
#include <window_dlg_wait.hpp>

/* Once is Enclosure enabled in menu with ON/OFF switch (MI_ENCLOSURE_ENABLED), it tests the fan and after that Enclosure is declared Active */
/* If test was passed, MI_ENCLOSURE_ENABLE is swapped with MI_ENCLOSURE and enclosure settings can be accessed */
/* This hides enclosure settings for Users without enclosure */
MI_ENCLOSURE::MI_ENCLOSURE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, xl_enclosure.isActive() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_ENCLOSURE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuEnclosure>);
}

MI_ENCLOSURE_ENABLE::MI_ENCLOSURE_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isEnabled(), _(label), nullptr, is_enabled_t::yes, xl_enclosure.isActive() ? is_hidden_t::yes : is_hidden_t::no) {
    /* Swapping of Menu Items MI_ENCLOSURE_ENABLE and MI_ENCLOSURE (See comment above) */
}

void MI_ENCLOSURE_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    xl_enclosure.setEnabled(value());
    if (value()) {
        /* Wait until enclosure is initialized & ready (test takes 3s). If initialization fails, the enclosure gets disabled internally. */
        gui_dlg_wait([] {
            if (xl_enclosure.isActive() || !xl_enclosure.isEnabled()) {
                Screens::Access()->Close();
            }
        },
            _(wait_str));
        if (!xl_enclosure.isEnabled()) {
            set_value(0, false);
        }
    }
}

MI_ENCLOSURE_TEMP::MI_ENCLOSURE_TEMP()
    : WI_FORMATABLE_LABEL_t<int>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
        if (const auto temp = xl_enclosure.getEnclosureTemperature()) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%u\xC2\xB0\x43", (uint8_t)std::clamp<buddy::Temperature>(*temp, 0, 99));
        } else {
            strlcpy(buffer, "--", GuiDefaults::infoDefaultLen);
        }
    }) {
}

MI_ENCLOSURE_PRINT_FILTRATION::MI_ENCLOSURE_PRINT_FILTRATION()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isPrintFiltrationEnabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_PRINT_FILTRATION::OnChange([[maybe_unused]] size_t old_index) {
    xl_enclosure.setPrintFiltration(value());
}

MI_ENCLOSURE_POST_PRINT_FILTRATION::MI_ENCLOSURE_POST_PRINT_FILTRATION()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isPostPrintFiltrationEnabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_POST_PRINT_FILTRATION::OnChange([[maybe_unused]] size_t old_index) {
    xl_enclosure.setPostPrintFiltration(value());
}

MI_ENCLOSURE_FILTER_COUNTER::MI_ENCLOSURE_FILTER_COUNTER()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {

    ArrayStringBuilder<16> info_text_builder;

    if (xl_enclosure.isExpirationShown()) {
        info_text_builder.append_string_view(_(text_expired));
    } else {
        uint32_t hours = config_store().xl_enclosure_filter_timer.get() / 3600;
        info_text_builder.append_printf("%" PRIu32 " h", hours);
    }
    ChangeInformation(info_text_builder.str());
}

static constexpr NumericInputConfig enclosure_fan_spin_config {
    .min_value = xl_enclosure.MIN_FAN_PWM,
    .max_value = 100,
    .step = 10.0F,
    .unit = Unit::percent,
};

MI_ENCLOSURE_FAN_SETTING::MI_ENCLOSURE_FAN_SETTING()
    : WiSpin(config_store().xl_enclosure_fan_manual.get(), enclosure_fan_spin_config, _(label), &img::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_FAN_SETTING::OnClick() {
    xl_enclosure.setUserFanRPM(GetVal());
}

static constexpr NumericInputConfig enclosure_post_print_spin_config {
    .min_value = 1,
    .max_value = 10,
    .unit = Unit::minute,
};

MI_ENCLOSURE_POST_PRINT_DURATION::MI_ENCLOSURE_POST_PRINT_DURATION()
    : WiSpin(config_store().xl_enclosure_post_print_duration.get(), enclosure_post_print_spin_config, _(label), &img::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_POST_PRINT_DURATION::OnClick() {
    xl_enclosure.setPostPrintFiltrationDuration(GetVal());
}

MI_ENCLOSURE_MANUAL_SETTINGS::MI_ENCLOSURE_MANUAL_SETTINGS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_ENCLOSURE_MANUAL_SETTINGS::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuManualSetting>);
}

MI_ENCLOSURE_FILTER_CHANGE::MI_ENCLOSURE_FILTER_CHANGE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_FILTER_CHANGE::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeFilter>);
}
