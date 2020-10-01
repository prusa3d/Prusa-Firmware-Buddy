// screen_menu_temperature.c

// Beware (especially on MS Windows, where diacritics still doesn't work as intended for more than 40 years)
// THIS FILE MUST BE SAVED as UTF-8 (which is normal on linux) and you need a sane compiler which
// accepts utf-8 literals (which gcc does)

#include "gui.hpp"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "ScreenHandler.hpp"

class MI_LangBase : public WI_LABEL_t {
public:
    inline MI_LangBase(const char *label, uint16_t icon_id)
        : WI_LABEL_t(label, icon_id, true, false) {}

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        LangEEPROM::getInstance().setLanguage(LangCode());
        Screens::Access()->Close();
    }
    virtual uint16_t LangCode() const = 0;

    virtual void printIcon(IWindowMenu &window_menu, Rect16 rect, uint8_t swap, color_t color_back) const override {
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
    // Beware UTF-8!
    static constexpr const char *const label = "Čeština";

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
    // Beware UTF-8!
    static constexpr const char *const label = "Español";

public:
    inline MI_SPANISH()
        : MI_LangBase(label, IDR_PNG_flag_es) {}

protected:
    virtual uint16_t LangCode() const override { return Translations::MakeLangCode("es"); }
};

class MI_FRENCH : public MI_LangBase {
    // Beware UTF-8!
    static constexpr const char *const label = "Français";

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
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;

class ScreenMenuLanguages : public Screen {
public:
    constexpr static const char *label = N_("LANGUAGES");
    ScreenMenuLanguages()
        : Screen(_(label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuLanguages() {
    return ScreenFactory::Screen<ScreenMenuLanguages>();
}

/*****************************************************************************/
//parent alias
using Screen_noReturn = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_ENGLISH, MI_CZECH, MI_GERMAN, MI_SPANISH, MI_FRENCH, MI_ITALIAN, MI_POLISH>;

class ScreenMenuLanguagesNoRet : public Screen_noReturn {
public:
    constexpr static const char *label = N_("SELECT LANGUAGE");
    ScreenMenuLanguagesNoRet()
        : Screen_noReturn(_(label)) {
        window_frame_t::ClrMenuTimeoutClose();
        window_frame_t::ClrOnSerialClose(); // don't close on Serial print
    }
};

ScreenFactory::UniquePtr GetScreenMenuLanguagesNoRet() {
    return ScreenFactory::Screen<ScreenMenuLanguagesNoRet>();
}
