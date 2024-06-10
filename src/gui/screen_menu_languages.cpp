/**
 * @file screen_menu_languages.cpp
 */

#include "screen_menu_languages.hpp"
#include "marlin_client.hpp"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "ScreenHandler.hpp"
#include "DialogMoveZ.hpp"
#include "img_resources.hpp"

MI_LangBase::MI_LangBase(const char *label, const img::Resource *icon, is_hidden_t hidden)
    : IWindowMenuItem(_(label), icon, is_enabled_t::yes, hidden) {
    setLabelFont(GuiDefaults::FontMenuSpecial);
}

void MI_LangBase::click(IWindowMenu & /*window_menu*/) {
    LangEEPROM::getInstance().setLanguage(LangCode());
    Screens::Access()->Close();
}

void MI_LangBase::printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const {
    raster_op.swap_bw = has_swapped_bw::no;
    render_icon_align(icon_rect, id_icon, color_back, icon_flags(Align_t::Center(), raster_op));
}

MI_ENGLISH::MI_ENGLISH()
    : MI_LangBase(label, &img::flag_en_16x11, is_hidden_t::no) {}

uint16_t MI_ENGLISH::LangCode() const {
    return Translations::MakeLangCode("en");
}

MI_CZECH::MI_CZECH()
    : MI_LangBase(label, &img::flag_cs_16x11, is_hidden_t::no) {}

uint16_t MI_CZECH::LangCode() const {
    return Translations::MakeLangCode("cs");
}

MI_GERMAN::MI_GERMAN()
    : MI_LangBase(label, &img::flag_de_16x11, is_hidden_t::no) {}

uint16_t MI_GERMAN::LangCode() const {
    return Translations::MakeLangCode("de");
}

MI_SPANISH::MI_SPANISH()
    : MI_LangBase(label, &img::flag_es_16x11, is_hidden_t::no) {}

uint16_t MI_SPANISH::LangCode() const {
    return Translations::MakeLangCode("es");
}

MI_FRENCH::MI_FRENCH()
    : MI_LangBase(label, &img::flag_fr_16x11, is_hidden_t::no) {}

uint16_t MI_FRENCH::LangCode() const {
    return Translations::MakeLangCode("fr");
}

MI_ITALIAN::MI_ITALIAN()
    : MI_LangBase(label, &img::flag_it_16x11, is_hidden_t::no) {}

uint16_t MI_ITALIAN::LangCode() const {
    return Translations::MakeLangCode("it");
}

MI_POLISH::MI_POLISH()
    : MI_LangBase(label, &img::flag_pl_16x11, is_hidden_t::no) {}

uint16_t MI_POLISH::LangCode() const {
    return Translations::MakeLangCode("pl");
}

MI_JAPANESE::MI_JAPANESE()
    : MI_LangBase(label, &img::flag_ja_16x11, is_hidden_t::no) {}

uint16_t MI_JAPANESE::LangCode() const {
    return Translations::MakeLangCode("ja");
}

MI_TEST_LANG::MI_TEST_LANG()
    : MI_LangBase(label, &img::flag_cs_16x11, is_hidden_t::dev) {}

uint16_t MI_TEST_LANG::LangCode() const {
    return Translations::MakeLangCode("ts");
}

ScreenMenuLanguages::ScreenMenuLanguages()
    : ScreenMenuLanguages__(_(label)) {
    EnableLongHoldScreenAction();
    header.SetIcon(&img::language_white_16x16);
}

ScreenMenuLanguagesNoRet::ScreenMenuLanguagesNoRet()
    : ScreenMenuLanguagesNoReturn__(_(label)) {
    window_frame_t::ClrMenuTimeoutClose();
    window_frame_t::ClrOnSerialClose(); // don't close on Serial print
}

void ScreenMenuLanguagesNoRet::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
