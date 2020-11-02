#include "screen_menus.hpp"
#include "gui.hpp"
#include "app.h"
#include "screen_menu.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include <type_traits>
#include "eeprom.h"
#include "screen_sheet_rename.hpp"
#include "wizard/screen_wizard.hpp"
#include "dbg.h"
#include "marlin_client.h"

class ScreenMenuSteelSheets;

enum class profile_action : uint32_t {
    Select = 1,
    Calibrate = 2,
    Reset = 3,
    Rename = 4
};

using sheet_index_0 = std::integral_constant<uint32_t, 0>;
using sheet_index_1 = std::integral_constant<uint32_t, 1>;
using sheet_index_2 = std::integral_constant<uint32_t, 2>;
using sheet_index_3 = std::integral_constant<uint32_t, 3>;
using sheet_index_4 = std::integral_constant<uint32_t, 4>;
using sheet_index_5 = std::integral_constant<uint32_t, 5>;
using sheet_index_6 = std::integral_constant<uint32_t, 6>;
using sheet_index_7 = std::integral_constant<uint32_t, 7>;

class MI_SHEET_SELECT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Select");

public:
    MI_SHEET_SELECT()
        : WI_LABEL_t(_(label), 0, false, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Select);
    }
};

class MI_SHEET_CALIBRATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("First Layer Calibration");

public:
    MI_SHEET_CALIBRATE()
        : WI_LABEL_t(_(label), 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Calibrate);
    }
};

class MI_SHEET_RENAME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Rename");

public:
    MI_SHEET_RENAME()
        : WI_LABEL_t(_(label), 0, true, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Rename);
    }
};

class MI_SHEET_RESET : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset");

public:
    MI_SHEET_RESET()
        : WI_LABEL_t(_(label), 0, false, false) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Reset);
    }
};

using SheetProfileMenuScreen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_SHEET_SELECT, MI_SHEET_CALIBRATE,
#if _DEBUG //todo remove #if _DEBUG after rename is finished
    MI_SHEET_RENAME,
#endif // _DEBUG
    MI_SHEET_RESET>;

template <typename Index>
class SheetProfileMenuScreenT : public SheetProfileMenuScreen {
public:
    constexpr static const char *label = N_("Sheet Profile");
    using index_type = Index;
    SheetProfileMenuScreenT()
        : SheetProfileMenuScreen(_(label)) {
        if (sheet_is_calibrated(Index::value)) {
            Item<MI_SHEET_SELECT>().Enable();
            Item<MI_SHEET_RESET>().Enable();
        }
        if (Index::value == 0)
            Item<MI_SHEET_RESET>().Disable();
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override {
        _dbg("SheetProfile::event");
        if (ev != GUI_event_t::CHILD_CLICK) {
            SuperWindowEvent(sender, ev, param);
            return;
        }
        profile_action action = static_cast<profile_action>((uint32_t)param);
        switch (action) {
        case profile_action::Reset:
            if (sheet_reset(Index::value)) {
                Item<MI_SHEET_RESET>().Disable();
                Item<MI_SHEET_SELECT>().Disable();
                _dbg("MI_SHEET_RESET OK");
            } else
                _dbg("MI_SHEET_RESET FAIL!");
            break;
        case profile_action::Select:
            _dbg("MI_SHEET_SELECT");
            sheet_select(Index::value);
            marlin_settings_load();
            break;
        case profile_action::Calibrate:
            _dbg("MI_SHEET_CALIBRATE");
            sheet_calibrate(Index::value);
            ScreenWizard::RunFirstLay();
            break;
        case profile_action::Rename:
            _dbg("MI_SHEET_RENAME");
#if _DEBUG //todo remove #if _DEBUG after rename is finished
            Screens::Access()->Open([]() {
                screen_sheet_rename_t::index(Index::value);
                return ScreenFactory::Screen<screen_sheet_rename_t>();
            });
#endif // _DEBUG
            break;
        default:
            _dbg("Click: %d\n", static_cast<uint32_t>(action));
            break;
        }
    }
};

template <typename Index>
struct profile_record_t : public WI_LABEL_t {
    char name[MAX_SHEET_NAME_LENGTH];
    profile_record_t()
        : WI_LABEL_t(string_view_utf8::MakeNULLSTR(), 0, true, false) {
        memset(name, 0, MAX_SHEET_NAME_LENGTH);
        sheet_name(Index::value, name, MAX_SHEET_NAME_LENGTH);
        // string_view_utf8::MakeRAM is safe. "name" is member var, exists until profile_record_t is destroyed
        SetLabel(string_view_utf8::MakeRAM((const uint8_t *)name));
    };

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open([]() {
            return ScreenFactory::Screen<SheetProfileMenuScreenT<Index>>();
        });
    }
};

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    ,
    profile_record_t<sheet_index_0>, profile_record_t<sheet_index_1>, profile_record_t<sheet_index_2>, profile_record_t<sheet_index_3>, profile_record_t<sheet_index_4>, profile_record_t<sheet_index_5>, profile_record_t<sheet_index_6>, profile_record_t<sheet_index_7>
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
