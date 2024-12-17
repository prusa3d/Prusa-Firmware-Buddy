#include "screen_menu_steel_sheets.hpp"
#include <type_traits>
#include <logging/log.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include "ScreenHandler.hpp"
#include "SteelSheets.hpp"
#include <marlin_client.hpp>
#include <dialog_text_input.hpp>
#include <str_utils.hpp>

void MI_SHEET_OFFSET::printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const {
    if (calib) {
        WI_LAMBDA_LABEL_t::printExtension(extension_rect, color_text, color_back, raster_op);
    } else {

        auto stringView = _(notCalibrated);
        render_text_align(extension_rect, stringView, InfoFont, color_back,
            (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter());
    }
}

MI_SHEET_OFFSET::MI_SHEET_OFFSET()
    : WI_LAMBDA_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::yes, [this](const std::span<char> &buffer) {
        snprintf(buffer.data(), buffer.size(), "%.3f mm", static_cast<double>(this->offset));
    }) {
}

void MI_SHEET_OFFSET::SetOffset(float offs) {
    calib = true;
    offset = offs;
}

void MI_SHEET_OFFSET::Reset() {
    calib = false;
}

MI_SHEET_SELECT::MI_SHEET_SELECT()
    : IWindowMenuItem(_(label)) {};

void MI_SHEET_SELECT::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Select);
}

MI_SHEET_CALIBRATE::MI_SHEET_CALIBRATE()
    : IWindowMenuItem(_(label)) {};

void MI_SHEET_CALIBRATE::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Calibrate);
}

MI_SHEET_RENAME::MI_SHEET_RENAME()
    : IWindowMenuItem(_(label)) {};

void MI_SHEET_RENAME::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Rename);
}

MI_SHEET_RESET::MI_SHEET_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no) {};

void MI_SHEET_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Reset);
}

SheetProfileMenuScreen::SheetProfileMenuScreen(uint32_t value)
    : SheetProfileMenuScreen__({})
    , value(value) {
    if (SteelSheets::IsSheetCalibrated(value)) {
        Item<MI_SHEET_SELECT>().set_enabled(true);
        Item<MI_SHEET_RESET>().set_enabled(true);
        Item<MI_SHEET_OFFSET>().SetOffset(SteelSheets::GetSheetOffset(value));
        Item<MI_SHEET_OFFSET>().set_is_hidden(false);
    }
    if (value == 0) {
        Item<MI_SHEET_RESET>().set_enabled(false);
    }

    update_title();
}

void SheetProfileMenuScreen::update_title() {
    StringBuilder sb(label_buffer);
    sb.append_string_view(_("Sheet: "));
    SteelSheets::SheetName(value, std::span<char, SHEET_NAME_BUFFER_SIZE>(sb.alloc_chars(MAX_SHEET_NAME_LENGTH), SHEET_NAME_BUFFER_SIZE));

    header.SetText(string_view_utf8::MakeRAM(label_buffer.data()));
}

void SheetProfileMenuScreen::windowEvent(window_t *sender, GUI_event_t ev, void *param) {
    if (ev != GUI_event_t::CHILD_CLICK) {
        ScreenMenu::windowEvent(sender, ev, param);
        return;
    }
    profile_action action = static_cast<profile_action>((uint32_t)param);
    switch (action) {

    case profile_action::Reset:
        if (SteelSheets::ResetSheet(value)) {
            Item<MI_SHEET_RESET>().set_enabled(false);
            Item<MI_SHEET_SELECT>().set_enabled(false);
            Item<MI_SHEET_OFFSET>().Reset();
        }
        break;

    case profile_action::Select:
        SteelSheets::SelectSheet(value);
        break;

    case profile_action::Calibrate: {
        const auto original_sheet = config_store().active_sheet.get();
        SteelSheets::SelectSheet(value);
        marlin_client::test_start_with_data(stmFirstLayer, selftest::FirstLayerCalibrationData { .previous_sheet = original_sheet });
        Item<MI_SHEET_OFFSET>().SetOffset(SteelSheets::GetSheetOffset(value));
    } break;

    case profile_action::Rename: {
        std::array<char, SHEET_NAME_BUFFER_SIZE> new_sheet_name;
        SteelSheets::SheetName(value, new_sheet_name);

        if (DialogTextInput::exec(_("Sheet name"), new_sheet_name)) {
            SteelSheets::RenameSheet(value, new_sheet_name.data(), new_sheet_name.size());
            update_title();
        }
        break;
    }

    default:
        break;
    }
}

ScreenMenuSteelSheets::ScreenMenuSteelSheets()
    : ScreenMenuSteelSheets__(_(label)) {
}

I_MI_SHEET_PROFILE::I_MI_SHEET_PROFILE(int sheet_index)
    : IWindowMenuItem({}, Rect16::W_t(16), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , sheet_index(sheet_index) {

    SteelSheets::SheetName(sheet_index, label_str);
    // string_view_utf8::MakeRAM is safe. "label_str" is a member var and exists until I_MI_SHEET_PROFILE is destroyed
    SetLabel(string_view_utf8::MakeRAM(label_str.data()));
}

// ScreenFactory::Screen depends on ProfileRecord following code cannot stay in header (be template)
void I_MI_SHEET_PROFILE::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<SheetProfileMenuScreen>(sheet_index));
}
