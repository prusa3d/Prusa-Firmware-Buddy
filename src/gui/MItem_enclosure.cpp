
#include "MItem_enclosure.hpp"
#include "img_resources.hpp"
#include "menu_spin_config.hpp"
#include "xl_enclosure.hpp"
#include "ScreenHandler.hpp"
#include "screen_change_filter.hpp"

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
        const auto temp = xl_enclosure.getEnclosureTemperature();
        if (temp != Enclosure::INVALID_TEMPERATURE) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%u\xC2\xB0\x43", (uint8_t)std::clamp(temp, 0, 99));
        } else {
            strlcpy(buffer, "--", GuiDefaults::infoDefaultLen);
        }
    }) {
}

MI_ENCLOSURE_ALWAYS_ON::MI_ENCLOSURE_ALWAYS_ON()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isAlwaysOnEnabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_ALWAYS_ON::OnChange([[maybe_unused]] size_t old_index) {
    xl_enclosure.setAlwaysOn(value());
}

MI_ENCLOSURE_POST_PRINT::MI_ENCLOSURE_POST_PRINT()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isPostPrintEnabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_POST_PRINT::OnChange([[maybe_unused]] size_t old_index) {
    xl_enclosure.setPostPrint(value());
}

MI_ENCLOSURE_FILTER_COUNTER::MI_ENCLOSURE_FILTER_COUNTER()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
    static char time_info[10] = "--";
    uint32_t hours = config_store().xl_enclosure_filter_timer.get() / 3600;
    snprintf(time_info, sizeof(time_info), "%" PRIu32 " h", hours);
    ChangeInformation(_(time_info));
}

MI_ENCLOSURE_FAN_SETTING::MI_ENCLOSURE_FAN_SETTING()
    : WiSpinInt(config_store().xl_enclosure_fan_manual.get(), SpinCnf::enclosure_fan, _(label), &img::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_FAN_SETTING::OnClick() {
    xl_enclosure.setUserFanRPM(GetVal());
}

MI_ENCLOSURE_MANUAL_SETTINGS::MI_ENCLOSURE_MANUAL_SETTINGS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_ENCLOSURE_MANUAL_SETTINGS::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuManualSetting>);
}

MI_ENCLOSURE_FILTRATION::MI_ENCLOSURE_FILTRATION()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_ENCLOSURE_FILTRATION::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFiltration>);
}

MI_ENCLOSURE_FILTER_CHANGE::MI_ENCLOSURE_FILTER_CHANGE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_FILTER_CHANGE::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeFilter>);
}
