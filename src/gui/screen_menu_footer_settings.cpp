/**
 * @file screen_menu_footer_settings.cpp
 */

#include "screen_menu_footer_settings.hpp"
#include "footer_item_types.hpp"
#include "status_footer.hpp"
#include "WindowMenuSpin.hpp"
#include "footer_eeprom.hpp"
#include "DialogMoveZ.hpp"
#include "footer_def.hpp"
#include "utility_extensions.hpp"
#include <option/has_side_fsensor.h>

static_assert(ftrstd::to_underlying(footer::Item::none) == 0, "Current implementation relies on none being 0");

footer::Item I_MI_FOOTER::to_footer_item(size_t index) {
    if (index == 0 || index >= ftrstd::to_underlying(footer::Item::_count) - std::size(footer::disabled_items)) {
        return footer::Item::none; // "none" item uses index 0
    } else {
        // map index to the enum, but some items might be disabled
        footer::Item current_item = static_cast<footer::Item>(index);

        auto num_disabled_skipped = std::ranges::count_if(footer::disabled_items, [&current_item](const auto &elem) {
            return current_item >= elem;
        });

        int found_extra = 0;
        for (int step_forward = 0; step_forward < num_disabled_skipped; ++step_forward) {
            // we need to make sure that the 'next' item is also not disabled
            while (std::ranges::find(footer::disabled_items, static_cast<footer::Item>(ftrstd::to_underlying(current_item) + step_forward + found_extra + 1)) != std::end(footer::disabled_items)) { // while the next item is also disabled
                ++found_extra;
            }
        }

        current_item = static_cast<footer::Item>(ftrstd::to_underlying(current_item) + num_disabled_skipped + found_extra);

        return current_item;
    }
}

size_t I_MI_FOOTER::to_index(footer::Item item) {
    if (item >= footer::Item::_count) {
        return ftrstd::to_underlying(footer::Item::none);
    }

    if (std::ranges::find(footer::disabled_items, item) != std::end(footer::disabled_items)) {
        return ftrstd::to_underlying(footer::Item::none);
    }

    auto num_disabled_skipped = std::ranges::count_if(footer::disabled_items, [&item](const auto &elem) {
        return item >= elem;
    });

    return ftrstd::to_underlying(item) - num_disabled_skipped; // Other footer items are shifted by 1
}

I_MI_FOOTER::I_MI_FOOTER(const char *const label, int item_n)
    : WI_LAMBDA_SPIN(_(label),
        ftrstd::to_underlying(footer::Item::_count) - std::size(footer::disabled_items), // Count of available footers, "None" included
        nullptr, is_enabled_t::yes, is_hidden_t::no,
        to_index(StatusFooter::GetSlotInit(item_n)), // Currently selected item
        [&](char *buffer) {
            strlcpy(buffer, footer::to_string(to_footer_item(GetIndex())), GuiDefaults::infoDefaultLen);
        }) {
    // There is a bug that when a printer with 'disabled' Item in eeprom gets loaded, upon entering Footer Settings menu it shows 'None' (because of if in to_index) but doesn't update the footer accordingly...
    // After several attempts to fix this, I've decided that rather than updating the eeprom value & then not redrawing the footer, it's better to not even update the eeprom value.
    // This ends up in the footer item showing 'None', but the eeprom value still has the previous (disabled) item and the footer still shows it.
    // This is not a problem, because it can generally only happen upon 'change of configuration' which never happens for the customer (but could happen regularly to the developer, since there are some footer items that are Debug only)
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
        DialogMoveZ::Show();
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
