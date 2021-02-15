#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "ScreenHandler.hpp"
#include "dbg.h"

class MI_WIFI_RESET : public WI_LABEL_t {
    constexpr static const char *const label = N_("Reset ESP");

public:
    MI_WIFI_RESET()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu &) override {
      _dbg0("maslo - reset ESP");
    }
};

/*****************************************************************************/
//parent alias
using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN, MI_WIFI_RESET>;

class ScreenMenuWiFi : public Screen {
public:
    constexpr static const char *label = N_("WiFi");
    ScreenMenuWiFi()
        : Screen(_(label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuWiFi() {
    return ScreenFactory::Screen<ScreenMenuWiFi>();
}
