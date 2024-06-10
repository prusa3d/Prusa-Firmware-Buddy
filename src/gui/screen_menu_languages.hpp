/**
 * @file screen_menu_languages.hpp
 */
// Beware (especially on MS Windows, where diacritics still doesn't work as intended for more than 40 years)
// THIS FILE MUST BE SAVED as UTF-8 (which is normal on linux) and you need a sane compiler which
// accepts utf-8 literals (which gcc does)

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include <option/enable_translation_cs.h>
#include <option/enable_translation_de.h>
#include <option/enable_translation_es.h>
#include <option/enable_translation_fr.h>
#include <option/enable_translation_it.h>
#include <option/enable_translation_pl.h>
#include <option/enable_translation_ja.h>
#include <str_utils.hpp>
#include <img_resources.hpp>

class MI_LangBase : public IWindowMenuItem {
public:
    MI_LangBase(const char *label, const char *lang_code, const img::Resource *icon, is_hidden_t hidden);

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
    virtual void printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const override;

private:
    const uint16_t lang_code;
};

template <TemplateString name, TemplateString code, const img::Resource *icon, is_hidden_t hidden_ = is_hidden_t::no>
class MI_LANG : public MI_LangBase {
public:
    MI_LANG()
        : MI_LangBase(name.str, code.str, icon, hidden_) {}
};

using ScreenMenuLanguages__ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_LANG<"English"_tstr, "en"_tstr, &img::flag_en_16x11>
#if ENABLE_TRANSLATION_CS()
    ,
    MI_LANG<"Čeština"_tstr, "cs"_tstr, &img::flag_cs_16x11>
#endif
#if ENABLE_TRANSLATION_DE()
    ,
    MI_LANG<"Deutsch"_tstr, "de"_tstr, &img::flag_de_16x11>
#endif
#if ENABLE_TRANSLATION_ES()
    ,
    MI_LANG<"Español"_tstr, "es"_tstr, &img::flag_es_16x11>
#endif
#if ENABLE_TRANSLATION_FR()
    ,
    MI_LANG<"Français"_tstr, "fr"_tstr, &img::flag_fr_16x11>
#endif
#if ENABLE_TRANSLATION_IT()
    ,
    MI_LANG<"Italiano"_tstr, "it"_tstr, &img::flag_it_16x11>
#endif
#if ENABLE_TRANSLATION_PL()
    ,
    MI_LANG<"Polski"_tstr, "pl"_tstr, &img::flag_pl_16x11>
#endif
#if ENABLE_TRANSLATION_JA()
    ,
    MI_LANG<"ニホンゴ"_tstr, "ja"_tstr, &img::flag_ja_16x11>
#endif
    >;

class ScreenMenuLanguages : public ScreenMenuLanguages__ {
public:
    enum class Context {
        standard,
        initial_language_selection,
    };
    ScreenMenuLanguages(Context context = Context::standard);
};
