#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "WindowMenuItems.hpp"
#include "ScreenHandler.hpp"
#include "RAII.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "i18n.h"
#include "dbg.h"
#include "ff.h"

#include <stm32_port.h>
#include <esp_loader.h>
#include <lwesp_ll_buddy.h>
#include <netdev.h>
#include <http_lifetime.h>

#ifdef __cplusplus
}
#endif

#include <array>

#define BOOT_ADDRESS      0x00000ul
#define USER_ADDRESS      0x01000ul
#define BLANK1_ADDRESS    0x7e000ul
#define BLANK2_ADDRESS    0xfb000ul
#define INIT_DATA_ADDRESS 0xfc000ul
#define BLANK3_ADDRESS    0xfe000ul

enum class esp_upload_action : uint32_t {
    Initial = 0,
    Connect = 1,
    Start_flash = 2,
    Write_data = 3,
    Reset = 8,
};

struct esp_entry {
    uint32_t address;
    const char *filename;
    uint32_t size;
};

extern UART_HandleTypeDef huart6;

// ----------------------------------------------------------------
// ESP UPLOADER - FLASH SIZE
class MI_ESP_FLASH_ESP_AT
    : public WI_LABEL_t {
    constexpr static const char *const label = N_("FLASH ESP AT");

public:
    MI_ESP_FLASH_ESP_AT()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)esp_upload_action::Start_flash);
    }
};
// ----------------------------------------------------------------

using MenuContainer = WinMenuContainer<MI_RETURN, MI_ESP_FLASH_ESP_AT>;

class ScreenMenuESPUpdate : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("ESP FLASH");
    static constexpr size_t helper_lines = 8;
    static constexpr int helper_font = IDR_FNT_SPECIAL;
    static constexpr size_t buffer_length = 512;

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;
    std::array<esp_entry, 6> firmware_set;
    bool loopInProgress;
    FIL file_descriptor;
    esp_upload_action progress_state;
    esp_entry *current_file;
    uint32_t readCount;
    loader_stm32_config_t loader_config;

public:
    ScreenMenuESPUpdate();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    static inline uint16_t get_help_h() {
        return helper_lines * (resource_font(helper_font)->h);
    }
};

ScreenMenuESPUpdate::ScreenMenuESPUpdate()
    : AddSuperWindow<screen_t>()
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectScreen.Height()) - get_help_h(), GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes)
    , firmware_set({ { { .address = BOOT_ADDRESS, .filename = "/ESP/boot_v1.7.bin", .size = 0 },
          { .address = USER_ADDRESS, .filename = "/ESP/user1.1024.new.2.bin", .size = 0 },
          { .address = BLANK1_ADDRESS, .filename = "/ESP/blank.bin", .size = 0 },
          { .address = BLANK2_ADDRESS, .filename = "/ESP/blank.bin", .size = 0 },
          { .address = INIT_DATA_ADDRESS, .filename = "/ESP/esp_init_data_default_v08.bin", .size = 0 },
          { .address = BLANK3_ADDRESS, .filename = "/ESP/blank.bin", .size = 0 } } })
    , loopInProgress(false)
    , file_descriptor({ 0 })
    , progress_state(esp_upload_action::Initial)
    , current_file(firmware_set.begin())
    , readCount(0)
    , loader_config({
          .huart = &huart6,
          .port_io0 = GPIOE,
          .pin_num_io0 = GPIO_PIN_6,
          .port_rst = GPIOC,
          .pin_num_rst = GPIO_PIN_13,
      }) {
    header.SetText(_(label));
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list
    help.SetText(_("- ESP not connected. Make sure the files are in the ESP folder of the flash disk"));
}

ScreenFactory::UniquePtr GetScreenMenuESPUpdate() {
    return ScreenFactory::Screen<ScreenMenuESPUpdate>();
}

void ScreenMenuESPUpdate::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event != GUI_event_t::LOOP && event != GUI_event_t::CHILD_CLICK) {
        SuperWindowEvent(sender, event, param);
        return;
    }

    //loop might be blocking
    if (loopInProgress)
        return;
    AutoRestore<bool> AR(loopInProgress);
    loopInProgress = true;

    if (event == GUI_event_t::CHILD_CLICK) {
        esp_upload_action action = static_cast<esp_upload_action>((uint32_t)param);
        if (action == esp_upload_action::Start_flash) {
            esp_flash_initialize();
            for (esp_entry *chunk = firmware_set.begin();
                 chunk != firmware_set.end(); ++chunk) {
                if (f_open(&file_descriptor, chunk->filename, FA_READ) != FR_OK) {
                    break;
                }
                chunk->size = f_size(&file_descriptor);
                f_close(&file_descriptor);
            }
            progress_state = esp_upload_action::Connect;
        }
        return;
    } else {
        switch (progress_state) {
        case esp_upload_action::Connect: {
            esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
            _dbg("ESP boot connect");
            if (ESP_LOADER_SUCCESS == esp_loader_connect(&config)) {
                help.SetText(_("Successfully connected to ESP. \nDo not switch the Printer off nor remove the Flash disk."));
                progress_state = esp_upload_action::Start_flash;
            } else {
                help.SetText(_("Connecting with ESP module failed."));
            }
            break;
        }
        case esp_upload_action::Start_flash:
            _dbg("ESP Start flash %s", current_file->filename);
            if (f_open(&file_descriptor, current_file->filename, FA_READ) != FR_OK) {
                _dbg("ESP flash: Unable to open file %s", current_file->filename);
                break;
            }

            if (esp_loader_flash_start(
                    current_file->address,
                    current_file->size,
                    buffer_length)
                != ESP_LOADER_SUCCESS) {
                _dbg("ESP flash: Unable to start flash on address %0xld", current_file->address);
                f_close(&file_descriptor);
                break;
            } else {
                progress_state = esp_upload_action::Write_data;
            }
            break;
        case esp_upload_action::Write_data: {
            UINT readBytes = 0;
            uint8_t buffer[buffer_length];

            FRESULT res = f_read(&file_descriptor, buffer, sizeof(buffer), &readBytes);
            readCount += readBytes;
            _dbg("ESP read data %ld", readCount);
            if (res != FR_OK) {
                _dbg("ESP flash: Unable to read file %s", current_file->filename);
                readBytes = 0;
            }
            if (readBytes > 0) {
                if (esp_loader_flash_write(buffer, readBytes) != ESP_LOADER_SUCCESS) {
                    _dbg("ESP flash write FAIL");
                }
            } else {
                f_close(&file_descriptor);
                ++current_file;
                progress_state = current_file != firmware_set.end()
                    ? esp_upload_action::Start_flash
                    : esp_upload_action::Reset;
            }
            break;
        }
        case esp_upload_action::Reset:
            _dbg("ESP finished flahing");
            help.SetText(_("ESP succesfully flashed. \nWiFI initiation started."));
            esp_loader_flash_finish(true);
            esp_set_operating_mode(ESP_RUNNING_MODE);
            netdev_init_esp();
            httpd_reinit();
            progress_state = esp_upload_action::Initial;
            current_file = firmware_set.begin();
            readCount = 0;
            break;
        default:
            break;
        }
    }
}
