#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "printers.h"

class MI_CONNECT_ENABLED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *label = N_("Enabled");

public:
    MI_CONNECT_ENABLED()
        : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_CONNECT_ENABLED), string_view_utf8::MakeCPUFLASH((const uint8_t *)label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override {
        eeprom_set_var(EEVAR_CONNECT_ENABLED, variant8_bool(index));
        // Connect will catch up with new config in its next iteration
    }
};

using Screen = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_CONNECT_ENABLED>;

class ScreenMenuConnect : public Screen {
public:
    constexpr static const char *label = N_("PRUSA CONNECT");
    ScreenMenuConnect()
        : Screen(_(label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuConnect() {
    return ScreenFactory::Screen<ScreenMenuConnect>();
}
