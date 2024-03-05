
#include "Mitem_enclosure.hpp"
#include "img_resources.hpp"
#include "menu_spin_config.hpp"
#include "xl_enclosure.hpp"
#include "ScreenHandler.hpp"
#include "screen_change_filter.hpp"

MI_ENCLOSURE_ENABLE::MI_ENCLOSURE_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isEnabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ENCLOSURE_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    xl_enclosure.setEnabled(value());
    // Based on the result of the test we have to enable / disable this (enclosure cannot be enabled if fan is not connected properly)
}

MI_ENCLOSURE_TEMP::MI_ENCLOSURE_TEMP()
    : WI_FORMATABLE_LABEL_t<int>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
        const auto temp = xl_enclosure.getEnclosureTemperature();
        if (temp != Enclosure::INVALID_TEMPERATURE) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%u\xC2\xB0\x43", (uint8_t)std::clamp(temp, 0, 99));
        } else {
            strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
        }
    }) {
}

MI_ENCLOSURE_ALWAYS_ON::MI_ENCLOSURE_ALWAYS_ON()
    : WI_ICON_SWITCH_OFF_ON_t(xl_enclosure.isAlwaysOn(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
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
    config_store().xl_enclosure_fan_manual.set(GetVal());
    xl_enclosure.flagRPMChange();
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
