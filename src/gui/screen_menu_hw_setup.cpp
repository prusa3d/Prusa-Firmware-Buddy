
#include "screen_menus.hpp"
#include "gui.hpp"
#include "app.h"
#include "screen_menu.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

class ScreenMenuHwSetup;

ScreenFactory::UniquePtr GetScreenMenuHwSetup() {
    return ScreenFactory::Screen<ScreenMenuHwSetup>();
}

class MI_STEEL_SHEETS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Steel Sheets");

public:
    MI_STEEL_SHEETS()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuSteelSheets);
    }
};

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_STEEL_SHEETS>;

class ScreenMenuHwSetup : public Screen {
public:
    constexpr static const char *label = N_("HW Setup");
    ScreenMenuHwSetup()
        : Screen(_(label)) {
    }
};
