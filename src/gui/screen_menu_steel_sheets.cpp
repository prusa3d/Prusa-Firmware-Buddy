/**
 * @file screen_menu_steel_sheets.cpp
 */

#include "screen_menu_steel_sheets.hpp"
#include <type_traits>
#include "log.h"
#include "GuiDefaults.hpp"
#include "ScreenHandler.hpp"
#include "SteelSheets.hpp"
#include <marlin_client.hpp>

LOG_COMPONENT_REF(GUI);

#if _DEBUG // todo remove #if _DEBUG after rename is finished
    #include "screen_sheet_rename.hpp"
#endif

void MI_SHEET_OFFSET::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {
    if (calib) {
        WI_LAMBDA_LABEL_t::printExtension(extension_rect, color_text, color_back, raster_op);
    } else {

        auto stringView = _(notCalibrated);
        render_text_align(extension_rect, stringView, InfoFont, color_back,
            (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter());
    }
}

MI_SHEET_OFFSET::MI_SHEET_OFFSET()
    : WI_LAMBDA_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::yes, [this](char *buffer) {
        snprintf(buffer, GuiDefaults::infoDefaultLen, "%.3f mm", static_cast<double>(this->offset));
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
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {};

void MI_SHEET_SELECT::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Select);
}

MI_SHEET_CALIBRATE::MI_SHEET_CALIBRATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_SHEET_CALIBRATE::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Calibrate);
}

MI_SHEET_RENAME::MI_SHEET_RENAME()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_SHEET_RENAME::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Rename);
}

MI_SHEET_RESET::MI_SHEET_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {};

void MI_SHEET_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Reset);
}

ISheetProfileMenuScreen::ISheetProfileMenuScreen(uint32_t value)
    : SheetProfileMenuScreen__(_(label))
    , value(value) {
    if (SteelSheets::IsSheetCalibrated(value)) {
        Item<MI_SHEET_SELECT>().Enable();
        Item<MI_SHEET_RESET>().Enable();
        Item<MI_SHEET_OFFSET>().SetOffset(SteelSheets::GetSheetOffset(value));
        Show<MI_SHEET_OFFSET>();
    }
    if (value == 0) {
        Item<MI_SHEET_RESET>().Disable();
    }
}

void ISheetProfileMenuScreen::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) {
    log_debug(GUI, "SheetProfile::event");
    if (ev != GUI_event_t::CHILD_CLICK) {
        SuperWindowEvent(sender, ev, param);
        return;
    }
    profile_action action = static_cast<profile_action>((uint32_t)param);
    switch (action) {
    case profile_action::Reset:
        if (SteelSheets::ResetSheet(value)) {
            Item<MI_SHEET_RESET>().Disable();
            Item<MI_SHEET_SELECT>().Disable();
            Item<MI_SHEET_OFFSET>().Reset();
            log_debug(GUI, "MI_SHEET_RESET OK");
        } else {
            log_error(GUI, "MI_SHEET_RESET FAIL!");
        }
        break;
    case profile_action::Select:
        log_debug(GUI, "MI_SHEET_SELECT");
        SteelSheets::SelectSheet(value);
        break;
    case profile_action::Calibrate:
        log_debug(GUI, "MI_SHEET_CALIBRATE");
        SteelSheets::SelectSheet(value);
        marlin_client::test_start(stmFirstLayer);
        Item<MI_SHEET_OFFSET>().SetOffset(SteelSheets::GetSheetOffset(value));
        break;
    case profile_action::Rename:
        log_debug(GUI, "MI_SHEET_RENAME");
#if _DEBUG // todo remove #if _DEBUG after rename is finished
        screen_sheet_rename_t::index(value);
        Screens::Access()->Open([]() { return ScreenFactory::Screen<screen_sheet_rename_t>(); });
#endif // _DEBUG
        break;
    default:
        log_debug(GUI, "Click: %d\n", static_cast<uint32_t>(action));
        break;
    }
}

ScreenMenuSteelSheets::ScreenMenuSteelSheets()
    : ScreenMenuSteelSheets__(_(label)) {
}

void IProfileRecord::name_sheet(uint32_t value, char *buff) {
    SteelSheets::SheetName(value, buff, MAX_SHEET_NAME_LENGTH + 1);
}

// ScreenFactory::Screen depends on ProfileRecord following code cannot stay in header (be template)
void IProfileRecord::click_index(uint32_t index) {
    switch (index) {
    case 0:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_0>>);
        return;
    case 1:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_1>>);
        return;
    case 2:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_2>>);
        return;
    case 3:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_3>>);
        return;
    case 4:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_4>>);
        return;
    case 5:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_5>>);
        return;
    case 6:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_6>>);
        return;
    case 7:
        Screens::Access()->Open(ScreenFactory::Screen<SheetProfileMenuScreenT<sheet_index_7>>);
        return;
    }
}

IProfileRecord::IProfileRecord()
    : WI_LABEL_t(string_view_utf8::MakeNULLSTR(), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
