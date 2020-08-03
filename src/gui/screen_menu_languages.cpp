// screen_menu_temperature.c

#include "gui.hpp"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"

class MI_LangBase : public WI_LABEL_t {
public:
    inline MI_LangBase(const char *label, uint16_t icon_id)
        : WI_LABEL_t(label, icon_id, true, false) {}

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        LangEEPROM::getInstance().setLanguage(LangCode());
        screen_close();
    }
    virtual uint16_t LangCode() const = 0;

    virtual void printIcon(IWindowMenu &window_menu, rect_ui16_t rect, uint8_t swap, color_t color_back) const override {
        render_unswapable_icon_align(getIconRect(window_menu, rect), id_icon, color_back, RENDER_FLG(ALIGN_CENTER, swap));
    }
};

class MI_ENGLISH : public MI_LangBase {
    static constexpr const char *const label = "English";

public:
    inline MI_ENGLISH()
        : MI_LangBase(label, IDR_PNG_flag_en) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("en"); }
};

class MI_CZECH : public MI_LangBase {
    static constexpr const char *const label = "Cestina";

public:
    inline MI_CZECH()
        : MI_LangBase(label, IDR_PNG_flag_cs) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("cs"); }
};

class MI_GERMAN : public MI_LangBase {
    static constexpr const char *const label = "Deutsch";

public:
    inline MI_GERMAN()
        : MI_LangBase(label, IDR_PNG_flag_de) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("de"); }
};

class MI_SPANISH : public MI_LangBase {
    static constexpr const char *const label = "Espanol";

public:
    inline MI_SPANISH()
        : MI_LangBase(label, IDR_PNG_flag_es) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("es"); }
};

class MI_FRENCH : public MI_LangBase {
    static constexpr const char *const label = "Francais";

public:
    inline MI_FRENCH()
        : MI_LangBase(label, IDR_PNG_flag_fr) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("fr"); }
};

class MI_ITALIAN : public MI_LangBase {
    static constexpr const char *const label = "Italiano";

public:
    inline MI_ITALIAN()
        : MI_LangBase(label, IDR_PNG_flag_it) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("it"); }
};

class MI_POLISH : public MI_LangBase {
    static constexpr const char *const label = "Polski";

public:
    inline MI_POLISH()
        : MI_LangBase(label, IDR_PNG_flag_pl) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("pl"); }
};

/*****************************************************************************/
//parent alias
using parent = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;

class ScreenMenuLanguages : public parent {
public:
    constexpr static const char *label = N_("LANGUAGES");
    static void Init(screen_t *screen);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuLanguages::Init(screen_t *screen) {
    Create(screen, _(label));
}

screen_t screen_menu_languages = {
    0,
    0,
    ScreenMenuLanguages::Init,
    ScreenMenuLanguages::CDone,
    ScreenMenuLanguages::CDraw,
    ScreenMenuLanguages::CEvent,
    sizeof(ScreenMenuLanguages), //data_size
    nullptr,                     //pdata
};

screen_t *const get_scr_menu_languages() { return &screen_menu_languages; }

/*****************************************************************************/
//parent alias
using parent_noReturn = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;

class ScreenMenuLanguagesNoRet : public parent_noReturn {
public:
    constexpr static const char *label = N_("SELECT LANGUAGE");
    static void Init(screen_t *screen);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuLanguagesNoRet::Init(screen_t *screen) {
    Create(screen, _(label));
}

screen_t screen_menu_languages_noret = {
    0,
    0,
    ScreenMenuLanguagesNoRet::Init,
    ScreenMenuLanguagesNoRet::CDone,
    ScreenMenuLanguagesNoRet::CDraw,
    ScreenMenuLanguagesNoRet::CEvent,
    sizeof(ScreenMenuLanguagesNoRet), //data_size
    nullptr,                          //pdata
};

screen_t *const get_scr_menu_languages_noret() { return &screen_menu_languages_noret; }
