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
    MI_LangBase(const char *label, const img::Resource *icon, is_hidden_t hidden);

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
    virtual uint16_t LangCode() const = 0;

    virtual void printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const override;
};

class MI_ENGLISH : public MI_LangBase {
    static constexpr const char *const label = "English";

public:
    MI_ENGLISH();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_CZECH : public MI_LangBase {
    // Beware UTF-8!
    static constexpr const char *const label = "Čeština";

public:
    MI_CZECH();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_GERMAN : public MI_LangBase {
    static constexpr const char *const label = "Deutsch";

public:
    MI_GERMAN();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_SPANISH : public MI_LangBase {
    // Beware UTF-8!
    static constexpr const char *const label = "Español";

public:
    MI_SPANISH();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_FRENCH : public MI_LangBase {
    // Beware UTF-8!
    static constexpr const char *const label = "Français";

public:
    MI_FRENCH();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_ITALIAN : public MI_LangBase {
    static constexpr const char *const label = "Italiano";

public:
    MI_ITALIAN();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_POLISH : public MI_LangBase {
    static constexpr const char *const label = "Polski";

public:
    MI_POLISH();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_JAPANESE : public MI_LangBase {
    static constexpr const char *const label = "ニホンゴ";

public:
    MI_JAPANESE();

protected:
    virtual uint16_t LangCode() const override;
};

class MI_TEST_LANG : public MI_LangBase {
    static constexpr const char *const label = "Test";

public:
    MI_TEST_LANG();

protected:
    virtual uint16_t LangCode() const override;
};

/*****************************************************************************/
// parent alias
using ScreenMenuLanguages__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_ENGLISH,
#if ENABLE_TRANSLATION_CS()
    MI_CZECH,
#endif
#if ENABLE_TRANSLATION_DE()
    MI_GERMAN,
#endif
#if ENABLE_TRANSLATION_ES()
    MI_SPANISH,
#endif
#if ENABLE_TRANSLATION_FR()
    MI_FRENCH,
#endif
#if ENABLE_TRANSLATION_IT()
    MI_ITALIAN,
#endif
#if ENABLE_TRANSLATION_PL()
    MI_POLISH,
#endif
#if ENABLE_TRANSLATION_JA()
    MI_JAPANESE,
#endif
    MI_TEST_LANG>;

class ScreenMenuLanguages : public ScreenMenuLanguages__ {
public:
    constexpr static const char *label = N_("LANGUAGES");
    ScreenMenuLanguages();
};

/*****************************************************************************/
// parent alias
using ScreenMenuLanguagesNoReturn__ = ScreenMenu<EFooter::Off, MI_ENGLISH,
#if ENABLE_TRANSLATION_CS()
    MI_CZECH,
#endif
#if ENABLE_TRANSLATION_DE()
    MI_GERMAN,
#endif
#if ENABLE_TRANSLATION_ES()
    MI_SPANISH,
#endif
#if ENABLE_TRANSLATION_FR()
    MI_FRENCH,
#endif
#if ENABLE_TRANSLATION_IT()
    MI_ITALIAN,
#endif
#if ENABLE_TRANSLATION_PL()
    MI_POLISH,
#endif
#if ENABLE_TRANSLATION_JA()
    MI_JAPANESE,
#endif
    MI_TEST_LANG>;

class ScreenMenuLanguagesNoRet : public ScreenMenuLanguagesNoReturn__ {
protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("SELECT LANGUAGE");
    ScreenMenuLanguagesNoRet();
};
