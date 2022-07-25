#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "printers.h"
#include <Connect/connect.hpp>

using con::OnlineStatus;

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

class MI_CONNECT_STATUS : public WI_INFO_t {
    constexpr static const char *const label = N_("Status");

public:
    MI_CONNECT_STATUS()
        : WI_INFO_t(_(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::dev) {
    }
};

using Screen = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_CONNECT_ENABLED, MI_CONNECT_STATUS>;

#define S(STATUS, TEXT)                                    \
    case OnlineStatus::STATUS:                             \
        Item<MI_CONNECT_STATUS>().ChangeInformation(TEXT); \
        break;

class ScreenMenuConnect : public Screen {
public:
    constexpr static const char *label = N_("PRUSA CONNECT");
    ScreenMenuConnect()
        : Screen(_(label)) {
    }
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override {
        if (event == GUI_event_t::CHILD_CLICK || event == GUI_event_t::LOOP) {
            switch (con::last_status()) {
                S(Off, "Off");
                S(NoConfig, "No Config");
                S(NoDNS, "DNS");
                S(NoConnection, "Refused");
                S(Tls, "TLS");
                S(Auth, "Unauthorized");
                S(ServerError, "Srv error");
                S(InternalError, "Bug");
                S(NetworkError, "Net fail");
                S(Confused, "Protocol err");
                S(Ok, "Online");
            default:
                S(Unknown, "Unknown");
            }
        } else {
            SuperWindowEvent(sender, event, param);
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuConnect() {
    return ScreenFactory::Screen<ScreenMenuConnect>();
}
