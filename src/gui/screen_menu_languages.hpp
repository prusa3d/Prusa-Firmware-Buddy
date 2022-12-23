/**
 * @file screen_menu_languages.hpp
 */
// Beware (especially on MS Windows, where diacritics still doesn't work as intended for more than 40 years)
// THIS FILE MUST BE SAVED as UTF-8 (which is normal on linux) and you need a sane compiler which
// accepts utf-8 literals (which gcc does)

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

class MI_LangBase : public WI_LABEL_t {
public:
    MI_LangBase(const char *label, const png::Resource *icon);

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

class MI_TEST_LANG : public MI_LangBase {
    static constexpr const char *const label = "Test";

public:
    MI_TEST_LANG();

protected:
    virtual uint16_t LangCode() const override;
};

/*****************************************************************************/
//parent alias
#ifdef _DEBUG
using ScreenMenuLanguages__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH, MI_TEST_LANG>;
#else
using ScreenMenuLanguages__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;
#endif

class ScreenMenuLanguages : public ScreenMenuLanguages__ {
public:
    constexpr static const char *label = N_("LANGUAGES");
    ScreenMenuLanguages();
};

/*****************************************************************************/
//parent alias
using ScreenMenuLanguagesNoReturn__ = ScreenMenu<EFooter::Off, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;

class ScreenMenuLanguagesNoRet : public ScreenMenuLanguagesNoReturn__ {
public:
    constexpr static const char *label = N_("SELECT LANGUAGE");
    ScreenMenuLanguagesNoRet();
};
