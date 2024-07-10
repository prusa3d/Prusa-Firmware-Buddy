#include "MItem_steel_sheets.hpp"

#include <common/SteelSheets.hpp>

/*****************************************************************************/
// MI_CURRENT_PROFILE
MI_CURRENT_SHEET_PROFILE::MI_CURRENT_SHEET_PROFILE()
    : IWindowMenuItem(_(label), extension_width, nullptr, is_enabled_t::yes, SteelSheets::NumOfCalibrated() > 1 ? is_hidden_t::no : is_hidden_t::yes) {
}

void MI_CURRENT_SHEET_PROFILE::printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn) const {
    std::array<char, SHEET_NAME_BUFFER_SIZE + 2> nameBuf;
    char *name = nameBuf.data();
    *(name++) = '[';
    name += SteelSheets::ActiveSheetName(std::span(nameBuf).subspan<1, SHEET_NAME_BUFFER_SIZE>());
    *(name++) = ']';
    *(name++) = '\0';

    render_text_align(extension_rect, string_view_utf8::MakeRAM((uint8_t *)nameBuf.data()), font, color_back,
        is_focused() ? COLOR_ORANGE : color_text, GuiDefaults::MenuPaddingItems, Align_t::RightCenter(), false);
}

void MI_CURRENT_SHEET_PROFILE::click(IWindowMenu &) {
    SteelSheets::NextSheet();
    InValidateExtension();
}
