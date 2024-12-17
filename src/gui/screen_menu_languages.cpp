/**
 * @file screen_menu_languages.cpp
 */

#include "screen_menu_languages.hpp"
#include "marlin_client.hpp"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"

MI_LangBase::MI_LangBase(const char *label, const char *lang_code, const img::Resource *icon, is_hidden_t hidden)
    : IWindowMenuItem(_(label), icon, is_enabled_t::yes, hidden)
    , lang_code(Translations::MakeLangCode(lang_code)) {
}

void MI_LangBase::click(IWindowMenu & /*window_menu*/) {
    LangEEPROM::getInstance().setLanguage(lang_code);
    Screens::Access()->Close();
}

void MI_LangBase::printIcon(Rect16 icon_rect, ropfn raster_op, Color color_back) const {
    raster_op.swap_bw = has_swapped_bw::no;
    render_icon_align(icon_rect, id_icon, color_back, icon_flags(Align_t::Center(), raster_op));
}

ScreenMenuLanguages::ScreenMenuLanguages(Context context)
    : ScreenMenuLanguages__(_("LANGUAGES")) //
{
    header.SetIcon(&img::language_white_16x16);

    switch (context) {

    case Context::standard:
        EnableLongHoldScreenAction();
        break;

    case Context::initial_language_selection:
        header.SetText(_("SELECT LANGUAGE"));
        Item<MI_RETURN>().hide();
        window_frame_t::ClrMenuTimeoutClose();
        window_frame_t::ClrOnSerialClose();
        break;
    }
}
