#include "screen_menus.hpp"
#include "gui.hpp"
#include "app.h"
#include "screen_menu.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include <type_traits>
#include "Marlin/src/core/serial.h"
#include "eeprom.h"

class ScreenMenuSteelSheets;

enum class profile_action : uint32_t {
    Select = 1,
    Calibrate = 2,
    Reset = 3,
    Rename = 4
};

using sheet_index_0 = std::integral_constant<std::uint32_t, 0>;
using sheet_index_1 = std::integral_constant<std::uint32_t, 1>;
using sheet_index_2 = std::integral_constant<std::uint32_t, 2>;
using sheet_index_3 = std::integral_constant<std::uint32_t, 3>;
using sheet_index_4 = std::integral_constant<std::uint32_t, 4>;
using sheet_index_5 = std::integral_constant<std::uint32_t, 5>;
using sheet_index_6 = std::integral_constant<std::uint32_t, 6>;
using sheet_index_7 = std::integral_constant<std::uint32_t, 7>;

class MI_SHEET_SELECT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Select");

public:
    MI_SHEET_SELECT()
        : WI_LABEL_t(label, 0, false, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, WINDOW_EVENT_CHILD_CLICK, (void *)profile_action::Select);
    }
};

class MI_SHEET_CALIBRATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("First layer cal.");

public:
    MI_SHEET_CALIBRATE()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, WINDOW_EVENT_CHILD_CLICK, (void *)profile_action::Calibrate);
    }
};

class MI_SHEET_RENAME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Rename");

public:
    MI_SHEET_RENAME()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, WINDOW_EVENT_CHILD_CLICK, (void *)profile_action::Rename);
    }
};

class MI_SHEET_RESET : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset");

public:
    MI_SHEET_RESET()
        : WI_LABEL_t(label, 0, false, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, WINDOW_EVENT_CHILD_CLICK, (void *)profile_action::Reset);
    }
};

using SheetProfileMenuScreen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_SHEET_SELECT, MI_SHEET_CALIBRATE,
    MI_SHEET_RESET>;

template <typename Index>
class SheetProfileMenuScreenT : public SheetProfileMenuScreen {
public:
    constexpr static const char *label = N_("Sheet Profile");
    using index_type = Index;
    SheetProfileMenuScreenT()
        : SheetProfileMenuScreen(_(label)) {
        if (sheet_is_initialized(Index::value)) {
            Item<MI_SHEET_SELECT>().Enable();
            Item<MI_SHEET_RESET>().Enable();
        }
        if (Index::value == 0)
            Item<MI_SHEET_RESET>().Disable();
    }

    virtual void windowEvent(window_t *sender, uint8_t ev, void *param) override {
        SERIAL_ECHOLN("SheetProfile::event");
        if (ev != WINDOW_EVENT_CHILD_CLICK) {
            SheetProfileMenuScreen::windowEvent(sender, ev, param);
            return;
        }
        profile_action action = static_cast<profile_action>((uint32_t)param);
        switch (action) {
        case profile_action::Reset:
            if (sheet_reset(Index::value)) {
                Item<MI_SHEET_RESET>().Disable();
                Item<MI_SHEET_SELECT>().Disable();
                SERIAL_ECHOLN("MI_SHEET_RESET OK");
            } else
                SERIAL_ECHOLN("MI_SHEET_RESET FAIL!");
            break;
        case profile_action::Select:
            SERIAL_ECHOLN("MI_SHEET_SELECT");
            sheet_select(Index::value);
            break;
        case profile_action::Calibrate:
            SERIAL_ECHOLN("MI_SHEET_CALIBRATE");
            break;
        default:
            SERIAL_ECHOPAIR("Click: ", static_cast<uint32_t>(action));
            SERIAL_ECHOLN("");
            break;
        }
    }
};

template <typename T>
ScreenFactory::UniquePtr GetScreenMenuSheetProfile() {
    return ScreenFactory::Screen<SheetProfileMenuScreenT<T>>();
}

class MI_SHEET_SMOOTH1 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Smooth1");

public:
    MI_SHEET_SMOOTH1()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_0>);
    }
};

class MI_SHEET_SMOOTH2 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Smooth2");

public:
    MI_SHEET_SMOOTH2()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_1>);
    }
};

class MI_SHEET_TEXTUR1 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Textur1");

public:
    MI_SHEET_TEXTUR1()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_2>);
    }
};

class MI_SHEET_TEXTUR2 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Textur2");

public:
    MI_SHEET_TEXTUR2()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_3>);
    }
};

class MI_SHEET_CUSTOM1 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Custom1");

public:
    MI_SHEET_CUSTOM1()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_4>);
    }
};

class MI_SHEET_CUSTOM2 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Custom2");

public:
    MI_SHEET_CUSTOM2()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_5>);
    }
};

class MI_SHEET_CUSTOM3 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Custom3");

public:
    MI_SHEET_CUSTOM3()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_6>);
    }
};

class MI_SHEET_CUSTOM4 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Custom4");

public:
    MI_SHEET_CUSTOM4()
        : WI_LABEL_t(label, 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSheetProfile<sheet_index_7>);
    }
};

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    ,
    MI_SHEET_SMOOTH1, MI_SHEET_SMOOTH2,
    MI_SHEET_TEXTUR1, MI_SHEET_TEXTUR2, MI_SHEET_CUSTOM1, MI_SHEET_CUSTOM2, MI_SHEET_CUSTOM3, MI_SHEET_CUSTOM4
#endif
    >;

class ScreenMenuSteelSheets : public Screen {
public:
    constexpr static const char *label = N_("Steel Sheets");
    ScreenMenuSteelSheets()
        : Screen(_(label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuSteelSheets() {
    return ScreenFactory::Screen<ScreenMenuSteelSheets>();
}
