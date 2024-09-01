#include "screen_menu_footer_settings.hpp"
#include "footer_item_types.hpp"
#include "status_footer.hpp"
#include "WindowMenuSpin.hpp"
#include "footer_eeprom.hpp"
#include <screen_move_z.hpp>
#include "footer_def.hpp"
#include "utility_extensions.hpp"

#include <common/utils/algorithm_extensions.hpp>

static_assert(ftrstd::to_underlying(footer::Item::none) == 0, "Current implementation relies on none being 0");

I_MI_FOOTER::I_MI_FOOTER(int item)
    : MenuItemSelectMenu({})
    , item_(item) //
{
    SetLabel(_("Item %i").formatted(label_params_, item + 1));
    set_current_item(stdext::index_of(footer::item_list, StatusFooter::GetSlotInit(item_)));
}

int I_MI_FOOTER::item_count() const {
    return footer::item_list.size();
}

void I_MI_FOOTER::build_item_text(int index, const std::span<char> &buffer) const {
    _(footer::to_string(footer::item_list[index])).copyToRAM(buffer);
}

bool I_MI_FOOTER::on_item_selected([[maybe_unused]] int old_index, int new_index) {
    StatusFooter::SetSlotInit(item_, footer::item_list[new_index]);

    return true;
}

static constexpr const char *temp_align_values[] = {
    N_("Static"),
    N_("Static-left"),
    N_("Dynamic"),
};

MI_LEFT_ALIGN_TEMP::MI_LEFT_ALIGN_TEMP()
    : MenuItemSwitch(_("Temp. style"), temp_align_values, size_t(FooterItemHeater::GetDrawType())) {}

void MI_LEFT_ALIGN_TEMP::OnChange(size_t /*old_index*/) {
    FooterItemHeater::SetDrawType(footer::ItemDrawType(GetIndex()));
}

MI_SHOW_ZERO_TEMP_TARGET::MI_SHOW_ZERO_TEMP_TARGET()
    : WI_ICON_SWITCH_OFF_ON_t(FooterItemHeater::IsZeroTargetDrawn(),
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SHOW_ZERO_TEMP_TARGET::OnChange(size_t old_index) {
    old_index == 0 ? FooterItemHeater::EnableDrawZeroTarget() : FooterItemHeater::DisableDrawZeroTarget();
}

static constexpr NumericInputConfig footer_center_N_spin_config = {
    .max_value = PRINTER_IS_PRUSA_MINI() ? 3 : 5,
    .special_value = 0,
};

MI_FOOTER_CENTER_N::MI_FOOTER_CENTER_N()
    : WiSpin(uint8_t(FooterLine::GetCenterN()), footer_center_N_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FOOTER_CENTER_N::OnClick() {
    FooterLine::SetCenterN(value());
}

void ScreenMenuFooterSettings::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        open_move_z_screen();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

ScreenMenuFooterSettings::ScreenMenuFooterSettings()
    : ScreenMenuFooterSettings__(_(label)) {
    EnableLongHoldScreenAction();
}

ScreenMenuFooterSettingsAdv::ScreenMenuFooterSettingsAdv()
    : ScreenMenuFooterSettingsAdv__(_(label)) {
}
