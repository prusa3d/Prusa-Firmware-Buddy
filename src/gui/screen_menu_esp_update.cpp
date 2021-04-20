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
#include "string.h"

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "stm32_port.h"
#include "esp_loader.h"
// #include "example_common.h"
#include "esp_flasher.h"
#define HIGHER_BAUDRATE 115520

#define ESP_DESC_SIZE 128
#define ESP_DESC_ROWS 4

extern uint8_t is_flashing;
char plan_str[ESP_DESC_SIZE];
esp8266_binaries_t bin;

class ESP_FLASHER {
public:
    enum class Msg : uint8_t {
        NoMsg,
        ConnectOK,
        ConnectNOK
    };
    static Msg msg;
    static bool esp_connected;

    static void FlashESP_AT();
    static void Connect();
    static void Reset();
    static void WriteStr(char *str);
    static Msg ConsumeMsg();
    static uint8_t GetLinesInBuffer(char *str);
};
ESP_FLASHER::Msg ESP_FLASHER::msg = ESP_FLASHER::Msg::NoMsg;
bool ESP_FLASHER::esp_connected = false;
void ESP_FLASHER::Reset() {
    esp_connected = false;
    esp_loader_reset_target();
    char tmp_str1[] = "- ESP not connected\n";
    WriteStr(tmp_str1);
}
void ESP_FLASHER::Connect() {
    char tmp_str[ESP_DESC_SIZE];
    if (connect_to_target() == ESP_LOADER_SUCCESS) {
        // msg = Msg::ConnectOK;
        esp_connected = true;
        char tmp_str1[] = "- ESP Connected\n";
        strcpy(tmp_str, tmp_str1);
        _dbg0("SYNC DONe - connected with ESP");
    } else {
        // msg = Msg::ConnectNOK;
        char tmp_str1[] = "- ESP Connection FAILED\n";
        strcpy(tmp_str, tmp_str1);
        _dbg0("SYNC FAiLED");
    }
    WriteStr(tmp_str);
}
void ESP_FLASHER::FlashESP_AT() {
    if (esp_connected) {
        get_esp8266_binaries(&bin);
        esp_loader_error_t err;
        char tmp_str[ESP_DESC_SIZE];

        strcpy(tmp_str, "- Flashing blink.bin\n");
        WriteStr(tmp_str);
        err = flash_binary(bin.boot.data, bin.boot.size, bin.boot.addr);


        // strcpy(tmp_str, "- Flashing boot1.7.bin\n");
        // WriteStr(tmp_str);
        // err = flash_binary(bin.boot.data, bin.boot.size, bin.boot.addr);
//
        // strcpy(tmp_str, "- Flashing user partition\n");
        // WriteStr(tmp_str);
        // err = flash_binary(bin.user.data, bin.user.size, bin.user.addr);
//
        // strcpy(tmp_str, "- Flashing blank.bin\n");
        // WriteStr(tmp_str);
        // err = flash_binary(bin.blank1.data, bin.blank1.size, bin.blank1.addr);
//
        // strcpy(tmp_str, "- Flashing init_data.bin\n");
        // WriteStr(tmp_str);
        // err = flash_binary(bin.init_data.data, bin.init_data.size, bin.init_data.addr);
//
        // strcpy(tmp_str, "- Flashing blank2.bin\n");
        // WriteStr(tmp_str);
        // err = flash_binary(bin.blank2.data, bin.blank2.size, bin.blank2.addr);
//
        // strcpy(tmp_str, "- Flashing blank3.bin\n");
        // WriteStr(tmp_str);
        // err = flash_binary(bin.blank3.data, bin.blank3.size, bin.blank3.addr);
    }
}
uint8_t ESP_FLASHER::GetLinesInBuffer(char *str) {
    // count lines
    uint8_t i = 0;
    uint8_t lines = 0;
    char d;
    d = str[0];
    while (d != '\0') {
        if (d == '\n') {
            lines++;
        }
        i++;
        d = str[i];
    }
    return lines;
}
void ESP_FLASHER::WriteStr(char *str) {
    memset(plan_str, 0, ESP_DESC_SIZE);
    strcpy(plan_str, str);

    /*     char tmp_str1[ESP_DESC_SIZE]; */
    // strcpy(tmp_str1, plan_str);
    // strcat(tmp_str1, str);
    // uint8_t lines = GetLinesInBuffer(tmp_str1);
    // if (lines > uint8_t(ESP_DESC_ROWS)) {
    //
    //     // -- remove lines from buffer from start
    //     uint8_t lines_to_remove = lines - ESP_DESC_ROWS;
    //     _dbg0("lines to remove %d", lines_to_remove);
    //     uint8_t i = 0, j = 0, stop = 0;
    //     char d;
    //     d = tmp_str1[0];
    //     // char tmp_str2[ESP_DESC_SIZE];
    //     while (d != '\0' || stop == 0) {
    //         if (d == '\n') {
    //             lines++;
    //         }
    //         if (lines == lines_to_remove) {
    //             stop = 1;
    //             break;
    //             // tmp_str2[j] = char(d);
    //             // j++;
    //         }
    //         i = i+1;
    //         d = tmp_str1[i];
    //     }
    //     // _dbg0("new chars %s", tmp_str2);
    //     _dbg0("new chars %i", i);
    //     memset(plan_str, 0, ESP_DESC_SIZE);
    //     char *p = tmp_str1 + i;
    //     // strcat(tmp_str2, tmp_str1 + i);
    //     strcpy(plan_str, p);
    // } else {
    //     memset(plan_str, 0, ESP_DESC_SIZE);
    //     strcpy(plan_str, tmp_str1);
    /* } */

    /*     char tmp_str1[ESP_DESC_SIZE]; */
    // strcpy(tmp_str1, plan_str);
    // strcat(tmp_str1, str);
    // uint8_t lines = GetLinesInBuffer(tmp_str1);
    // if (lines > uint8_t(ESP_DESC_ROWS)) {
    //
    //     // -- remove lines from buffer from start
    //     uint8_t lines_to_remove = lines - ESP_DESC_ROWS;
    //     _dbg0("lines to remove %d", lines_to_remove);
    //     uint8_t i = 0, j = 0;
    //     char d;
    //     d = tmp_str1[0];
    //     char tmp_str2[ESP_DESC_SIZE];
    //     while (d != '\0') {
    //         if (d == '\n') {
    //             lines++;
    //         }
    //         if (lines > lines_to_remove) {
    //           tmp_str2[j] = char(d);
    //           j++;
    //         }
    //         i++;
    //         d = tmp_str1[i];
    //     }
    //     _dbg0("new chars %s", tmp_str2);
    //     memset(plan_str, 0, ESP_DESC_SIZE);
    //     // strcat(tmp_str2, tmp_str1 + i);
    //     strcpy(plan_str, tmp_str2);
    // } else {
    //     memset(plan_str, 0, ESP_DESC_SIZE);
    //     strcpy(plan_str, tmp_str1);
    /* } */
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
        ESP_FLASHER::Reset();
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

// ----------------------------------------------------------------
// ESP UPLOADER - FLASH SIZE
class MI_ESP_FLASH_ESP_AT
    : public WI_LABEL_t {
    constexpr static const char *const label = N_("FLASH ESP AT");

public:
    MI_ESP_FLASH_ESP_AT()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        ESP_FLASHER::FlashESP_AT();
    }
};
// ----------------------------------------------------------------

using MenuContainer = WinMenuContainer<MI_RETURN, MI_ESP_RESET, MI_ESP_CONNECT, MI_ESP_FLASH_ESP_AT>;

class ScreenMenuESPUpdate : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("ESP FLASH");
    static constexpr size_t helper_lines = 8;
    static constexpr int helper_font = IDR_FNT_SPECIAL;

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;

    bool msg_shown;
    void show_msg(ESP_FLASHER::Msg msg);
    void refresh_help_text();

public:
    ScreenMenuESPUpdate();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    static inline uint16_t get_help_h() {
        return helper_lines * (resource_font(helper_font)->h);
    }
};

ScreenMenuESPUpdate::ScreenMenuESPUpdate()
    : AddSuperWindow<screen_t>(nullptr, GuiDefaults::RectScreen, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectScreen.Height()) - get_help_h(), GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes) {
    header.SetText(_(label));
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list

    // -- write first line into a help text rect
    char init_line[] = "- ESP not connected\n";
    ESP_FLASHER::WriteStr(init_line);

    msg_shown = false;
}

ScreenFactory::UniquePtr GetScreenMenuESPUpdate() {
    return ScreenFactory::Screen<ScreenMenuESPUpdate>();
}

void ScreenMenuESPUpdate::refresh_help_text() {
    help.text = string_view_utf8::MakeRAM((const uint8_t *)plan_str);
    help.Invalidate();
    gui_invalidate();
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
    if (event == GUI_event_t::LOOP) {
        refresh_help_text();
    }
    show_msg(ESP_FLASHER::ConsumeMsg());
    SuperWindowEvent(sender, event, param);
}
