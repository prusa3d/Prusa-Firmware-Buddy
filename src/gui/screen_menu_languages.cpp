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
    inline MI_LangBase(const char *label)
        : WI_LABEL_t(label, 0, true, false) {}

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        LangEEPROM::getInstance().setLanguage(LangCode());
    }
    virtual uint16_t LangCode() const = 0;
};

class MI_ENGLISH : public MI_LangBase {
    static constexpr const char *const label = "English";

public:
    inline MI_ENGLISH()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("en"); }
};

class MI_CZECH : public MI_LangBase {
    static constexpr const char *const label = "Czech";

public:
    inline MI_CZECH()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("cs"); }
};

class MI_GERMAN : public MI_LangBase {
    static constexpr const char *const label = "German";

public:
    inline MI_GERMAN()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("de"); }
};

class MI_SPANISH : public MI_LangBase {
    static constexpr const char *const label = "Spanish";

public:
    inline MI_SPANISH()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("es"); }
};

class MI_FRENCH : public MI_LangBase {
    static constexpr const char *const label = "French";

public:
    inline MI_FRENCH()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("fr"); }
};

class MI_ITALIAN : public MI_LangBase {
    static constexpr const char *const label = "Italian";

public:
    inline MI_ITALIAN()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("it"); }
};

class MI_POLISH : public MI_LangBase {
    static constexpr const char *const label = "Polish";

public:
    inline MI_POLISH()
        : MI_LangBase(label) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("pl"); }
};

/*****************************************************************************/
//parent alias
using parent = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_CZECH, MI_GERMAN, MI_ENGLISH, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;

class ScreenMenuLanguages : public parent {
public:
    constexpr static const char *label = N_("LANGUAGES");
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuLanguages::Init(screen_t *screen) {
    Create(screen, _(label));
}

int ScreenMenuLanguages::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuLanguages *const ths = reinterpret_cast<ScreenMenuLanguages *>(screen->pdata);
    //    if (event == WINDOW_EVENT_CLICK) {
    //        marlin_set_target_nozzle(0);
    //        marlin_set_display_nozzle(0);
    //        marlin_set_target_bed(0);
    //        marlin_set_fan_speed(0);

    //        MI_NOZZLE *noz = &ths->Item<MI_NOZZLE>();
    //        MI_HEATBED *bed = &ths->Item<MI_HEATBED>();
    //        MI_PRINTFAN *fan = &ths->Item<MI_PRINTFAN>();
    //        noz->ClrVal();
    //        bed->ClrVal();
    //        fan->ClrVal();
    //    }
    return ths->Event(window, event, param);
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
