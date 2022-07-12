#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include <type_traits>
#include "eeprom.h"
#include "wizard/screen_wizard.hpp"
#include "log.h"
#include "SteelSheets.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "GuiDefaults.hpp"

#if _DEBUG // todo remove #if _DEBUG after rename is finished
    #include "screen_sheet_rename.hpp"
#endif

// TODO there is nowhere displayed currently selected sheet
// entire menu is confusing

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

class MI_SHEET_OFFSET : public WI_LAMBDA_LABEL_t {
    static constexpr const char *const label = N_("Offset");
    float offset = 0;
    bool calib = false;
    static constexpr char const *const notCalibrated = N_("Not Calib");
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override {
        if (calib) {
            WI_LAMBDA_LABEL_t::printExtension(extension_rect, color_text, color_back, raster_op);
        } else {

            auto stringView = _(notCalibrated);
            render_text_align(extension_rect, stringView, InfoFont, color_back,
                (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPadding, Align_t::RightCenter());
        }
    }

public:
    MI_SHEET_OFFSET()
        : WI_LAMBDA_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::yes, [this](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.3f mm", static_cast<double>(this->offset));
        }) {
    }

    void SetOffset(float offs) {
        calib = true;
        offset = offs;
    }
    void Reset() {
        calib = false;
    }
};

class MI_SHEET_SELECT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Select");

public:
    MI_SHEET_SELECT()
        : WI_LABEL_t(_(label), 0, is_enabled_t::no, is_hidden_t::no) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Select);
    }
};

class MI_SHEET_CALIBRATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("First Layer Calibration");

public:
    MI_SHEET_CALIBRATE()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Calibrate);
    }
};

class MI_SHEET_RENAME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Rename");

public:
    MI_SHEET_RENAME()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Rename);
    }
};

class MI_SHEET_RESET : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset");

public:
    MI_SHEET_RESET()
        : WI_LABEL_t(_(label), 0, is_enabled_t::no, is_hidden_t::no) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)profile_action::Reset);
    }
};

using SheetProfileMenuScreen = ScreenMenu<EFooter::On, MI_RETURN, MI_SHEET_SELECT, MI_SHEET_CALIBRATE,
#if _DEBUG // todo remove #if _DEBUG after rename is finished
    MI_SHEET_RENAME,
#endif // _DEBUG
    MI_SHEET_RESET, MI_SHEET_OFFSET>;

// TODO there is no way to tell which sheet I am currently calibrating
class ISheetProfileMenuScreen : public SheetProfileMenuScreen {
    uint32_t value;

public:
    constexpr static const char *label = N_("Sheet Profile");
    ISheetProfileMenuScreen(uint32_t value)
        : SheetProfileMenuScreen(_(label))
        , value(value) {
        if (SteelSheets::IsSheetCalibrated(value)) {
            Item<MI_SHEET_SELECT>().Enable();
            Item<MI_SHEET_RESET>().Enable();
            Item<MI_SHEET_OFFSET>().SetOffset(SteelSheets::GetSheetOffset(value));
            Show<MI_SHEET_OFFSET>();
        }
        if (value == 0)
            Item<MI_SHEET_RESET>().Disable();
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override {
        log_debug(GUI, "SheetProfile::event");
        if (ev != GUI_event_t::CHILD_CLICK) {
            SuperWindowEvent(sender, ev, param);
            return;
        }
        profile_action action = static_cast<profile_action>((uint32_t)param);
        switch (action) {
        case profile_action::Reset:
            if (SteelSheets::ResetSheet(value)) {
                Item<MI_SHEET_RESET>().Disable();
                Item<MI_SHEET_SELECT>().Disable();
                Item<MI_SHEET_OFFSET>().Reset();
                log_debug(GUI, "MI_SHEET_RESET OK");
            } else
                log_error(GUI, "MI_SHEET_RESET FAIL!");
            break;
        case profile_action::Select:
            log_debug(GUI, "MI_SHEET_SELECT");
            SteelSheets::SelectSheet(value);
            break;
        case profile_action::Calibrate:
            log_debug(GUI, "MI_SHEET_CALIBRATE");
            SteelSheets::SelectSheet(value);
            ScreenWizard::Run(wizard_run_type_t::firstlay);
            Item<MI_SHEET_OFFSET>().SetOffset(SteelSheets::GetSheetOffset(value));
            break;
        case profile_action::Rename:
            log_debug(GUI, "MI_SHEET_RENAME");
#if _DEBUG // todo remove #if _DEBUG after rename is finished
            screen_sheet_rename_t::index(value);
            Screens::Access()->Open([]() { return ScreenFactory::Screen<screen_sheet_rename_t>(); });
#endif // _DEBUG
            break;
        default:
            log_debug(GUI, "Click: %d\n", static_cast<uint32_t>(action));
            break;
        }
    }
};

template <typename Index>
class SheetProfileMenuScreenT : public ISheetProfileMenuScreen {
public:
    using index_type = Index;
    SheetProfileMenuScreenT()
        : ISheetProfileMenuScreen(Index::value) {
    }
};

template <typename Index>
struct ProfileRecord : public WI_LABEL_t {
    char name[MAX_SHEET_NAME_LENGTH + 1];
    ProfileRecord()
        : WI_LABEL_t(string_view_utf8::MakeNULLSTR(), 0, is_enabled_t::yes, is_hidden_t::no) {
        memset(name, 0, MAX_SHEET_NAME_LENGTH + 1);
        SteelSheets::SheetName(Index::value, name, MAX_SHEET_NAME_LENGTH + 1);
        // string_view_utf8::MakeRAM is safe. "name" is member var, exists until ProfileRecord is destroyed
        SetLabel(string_view_utf8::MakeRAM((const uint8_t *)name));
    };

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open([]() {
            return ScreenFactory::Screen<SheetProfileMenuScreenT<Index>>();
        });
    }
};

using Screen = ScreenMenu<EFooter::On, MI_RETURN
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    ,
    ProfileRecord<sheet_index_0>, ProfileRecord<sheet_index_1>, ProfileRecord<sheet_index_2>, ProfileRecord<sheet_index_3>, ProfileRecord<sheet_index_4>, ProfileRecord<sheet_index_5>, ProfileRecord<sheet_index_6>, ProfileRecord<sheet_index_7>
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
