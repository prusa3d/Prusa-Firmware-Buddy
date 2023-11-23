/**
 * @file screen_menu_footer_settings.cpp
 */

#include "screen_menu_footer_settings.hpp"
#include "footer_item_union.hpp"
#include "status_footer.hpp"
#include "menu_spin_config.hpp"
#include "footer_eeprom.hpp"
#include "DialogMoveZ.hpp"
#include "footer_def.hpp"
#include "utility_extensions.hpp"
#include <option/has_side_fsensor.h>

footer::Item I_MI_FOOTER::to_footer_item(size_t index) {
    if (index == 0 || index >= ftrstd::to_underlying(footer::Item::_count)) {
        return footer::Item::None; // "None" item uses index 0
    } else {
        return static_cast<footer::Item>(index - 1); // Other footer items are shifted by 1
    }
}

size_t I_MI_FOOTER::to_index(footer::Item item) {
    if (item == footer::Item::None) {
        return 0; // "None" item uses index 0
    } else {
        return ftrstd::to_underlying(item) + 1; // Other footer items are shifted by 1
    }
}

I_MI_FOOTER::I_MI_FOOTER(const char *const label, int item_n)
    : WI_LAMBDA_SPIN(_(label),
        ftrstd::to_underlying(footer::Item::_count), // Count of available footers, "None" included
        nullptr, is_enabled_t::yes, is_hidden_t::no,
        to_index(StatusFooter::GetSlotInit(item_n)), // Currently selected item
        [&](char *buffer) {
            strncpy(buffer, footer::to_string(to_footer_item(GetIndex())), GuiDefaults::infoDefaultLen);
        }) {
}

void I_MI_FOOTER::store_footer_index(size_t item_n) {
    StatusFooter::SetSlotInit(item_n, to_footer_item(GetIndex())); // Store footer
}

MI_LEFT_ALIGN_TEMP::MI_LEFT_ALIGN_TEMP()
    : WI_SWITCH_t(size_t(FooterItemHeater::GetDrawType()),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::no,
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_0),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_1),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_2)) {}

void MI_LEFT_ALIGN_TEMP::OnChange(size_t /*old_index*/) {
    FooterItemHeater::SetDrawType(footer::ItemDrawType(index));
}

MI_SHOW_ZERO_TEMP_TARGET::MI_SHOW_ZERO_TEMP_TARGET()
    : WI_ICON_SWITCH_OFF_ON_t(FooterItemHeater::IsZeroTargetDrawn(),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SHOW_ZERO_TEMP_TARGET::OnChange(size_t old_index) {
    old_index == 0 ? FooterItemHeater::EnableDrawZeroTarget() : FooterItemHeater::DisableDrawZeroTarget();
}

MI_FOOTER_CENTER_N::MI_FOOTER_CENTER_N()
    : WiSpinInt(uint8_t(FooterLine::GetCenterN()),
        SpinCnf::footer_center_N_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FOOTER_CENTER_N::OnClick() {
    FooterLine::SetCenterN(GetVal());
}

void ScreenMenuFooterSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenMenuFooterSettings::ScreenMenuFooterSettings()
    : ScreenMenuFooterSettings__(_(label)) {
    EnableLongHoldScreenAction();
}

ScreenMenuFooterSettingsAdv::ScreenMenuFooterSettingsAdv()
    : ScreenMenuFooterSettingsAdv__(_(label)) {
}
