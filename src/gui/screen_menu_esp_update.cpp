#include "sys.h"
#include "gui.hpp"
#include <stdbool.h>
#include "RAII.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "main.h"
#include "dbg.h"

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "stm32_port.h"
#include "esp_loader.h"
#include "example_common.h"
#define HIGHER_BAUDRATE 115520

extern uint8_t is_flashing;

class ESP_FLASHER {
public:
    enum class Msg : uint8_t {
        NoMsg,
        ConnectOK,
        ConnectNOK
    };
    static Msg msg;

    static void Connect();
    static Msg ConsumeMsg();
};
ESP_FLASHER::Msg ESP_FLASHER::msg = ESP_FLASHER::Msg::NoMsg;
void ESP_FLASHER::Connect() {
    if (connect_to_target(HIGHER_BAUDRATE) == ESP_LOADER_SUCCESS) {
        msg = Msg::ConnectOK;
        _dbg0("SYNC DONe - connected with ESP");
    } else {
        msg = Msg::ConnectNOK;
        _dbg0("SYNC FAiLED");
    }
}
ESP_FLASHER::Msg ESP_FLASHER::ConsumeMsg() {
    Msg ret = msg;
    msg = Msg::NoMsg;
    return ret;
}

// ----------------------------------------------------------------
// RESET
class MI_ESP_RESET : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP RESET");

public:
    MI_ESP_RESET()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [>window_menu<] */) override {
        esp_loader_reset_target();
    }
};
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// ESP UPLOADER - SYNC
class MI_ESP_CONNECT
    : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP CONNECT");

public:
    MI_ESP_CONNECT()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        ESP_FLASHER::Connect();
    }
};
// ----------------------------------------------------------------

using MenuContainer = WinMenuContainer<MI_RETURN, MI_ESP_RESET, MI_ESP_CONNECT>;

class ScreenMenuESPUpdate : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("ESP FLASH");

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    status_footer_t footer;

    bool msg_shown;
    void show_msg(ESP_FLASHER::Msg msg);

public:
    ScreenMenuESPUpdate();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

ScreenMenuESPUpdate::ScreenMenuESPUpdate()
    : AddSuperWindow<screen_t>(nullptr, GuiDefaults::RectScreen, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, GuiDefaults::RectScreenBody, &container)
    , header(this)
    , footer(this) {
    header.SetText(_(label));
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list

    msg_shown = false;
}

ScreenFactory::UniquePtr GetScreenMenuESPUpdate() {
    return ScreenFactory::Screen<ScreenMenuESPUpdate>();
}

void ScreenMenuESPUpdate::show_msg(ESP_FLASHER::Msg msg) {
    if (msg_shown) {
        return;
    }
    AutoRestore<bool> AR(msg_shown);
    msg_shown = true;
    switch (msg) {
    case ESP_FLASHER::Msg::ConnectOK:
        MsgBoxInfo(_("Succesfuly connected to ESP module and switched into UART bootloader."), Responses_Ok);
        break;
    case ESP_FLASHER::Msg::ConnectNOK:
        MsgBoxError(_("Connecting with ESP module failed."), Responses_Ok);
        break;
    default:
        break;
    }
}

void ScreenMenuESPUpdate::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    show_msg(ESP_FLASHER::ConsumeMsg());
    SuperWindowEvent(sender, event, param);
}
