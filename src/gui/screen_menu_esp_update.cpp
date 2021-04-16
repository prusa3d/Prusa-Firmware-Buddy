/*
 * screen_menu_fw_update.cpp
 *
 *  Created on: Dec 18, 2019
 *      Author: Migi
 */

#include "sys.h"
#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "main.h"
#include "dbg.h"
#include "lwesp_conn_upload.h"

extern UART_HandleTypeDef huart6;

// ----------------------------------------------------------------
// RESET
class MI_ESP_RESET : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP RESET");

public:
    MI_ESP_RESET()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [>window_menu<] */) override {
        // reset via GPIO pins LOW then HIGH
        HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_RESET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
        _dbg0("HW RESET");
    }
};
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// ESP UPLOADER - SYNC
class MI_ESP_SYNC : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP: sync");

public:
    MI_ESP_SYNC()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        lwespr_t eres;
        if ((eres = lwesp_conn_upload_start(NULL, NULL, NULL, 1)) == lwespOK) {
            // if (lwesp_set_wifi_mode(LWESP_MODE_STA, NULL, NULL, 1) == lwespOK) {
            _dbg0("POSLANO SYNC");
        } else if (eres == lwespTIMEOUT) {
            _dbg0("ESP TIMEOUT");
        } else if (eres == lwespERRNODEVICE) {
            _dbg0("ESP NO DEVICE");
        } else {
            _dbg0("ESP ERROR %d", eres);
        }
    }
};
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// ESP UPLOADER - READREG
class MI_ESP_READ : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP: read_reg");

public:
    MI_ESP_READ()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        lwespr_t eres;
        uint32_t addr = 0;
        // if (lwesp_conn_upload_start(NULL, NULL, NULL, 1) == lwespOK) {
        if ((eres = lwesp_set_wifi_mode(LWESP_MODE_STA, NULL, NULL, 1)) == lwespOK) {
            _dbg0("POSLANO READ na adrese: %x", addr);
        } else if (eres == lwespTIMEOUT) {
            _dbg0("READ REG ESP TIMEOUT");
        } else if (eres == lwespERRNODEVICE) {
            _dbg0("FLAS ESP NO DEVICE");
        } else {
            _dbg0("ESP ERROR %d", eres);
        }
    }
};
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// ESP UPLOADER - FLASH START
class MI_ESP_ERASE : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP: erase flash");

public:
    MI_ESP_ERASE()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        lwespr_t eres;
        uint32_t bin_size = 0x400;
        uint32_t offset = 0;
        if ((eres = lwesp_conn_upload_flash(NULL, NULL, bin_size, offset, NULL, 1)) == lwespOK) {
            _dbg0("POSLANO ERASE na adrese: %x o delce: %x", offset, bin_size);
        } else if (eres == lwespTIMEOUT) {
            _dbg0("FLAS ESP TIMEOUT");
        } else if (eres == lwespERRNODEVICE) {
            _dbg0("FLAS ESP NO DEVICE");
        } else {
            _dbg0("ESP ERROR %d", eres);
        }
    }
};
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// GPIO0 - UP/DOWN
class MI_ESP_FLASH : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("ESP FLASH");
    constexpr static const char *const s_on = N_("On");
    constexpr static const char *const s_off = N_("Off");

    size_t init_index() const;

public:
    MI_ESP_FLASH();

protected:
    virtual void OnChange(size_t) override;
};
MI_ESP_FLASH::MI_ESP_FLASH()
    : WI_SWITCH_t<2>(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no, _(s_on), _(s_off)) {}
size_t MI_ESP_FLASH::init_index() const {
    uint8_t gpio_state = (HAL_GPIO_ReadPin(GPIOE, (1 << (ESP_GPIO0_Pin & 0x0f))) != GPIO_PIN_RESET) ? 1 : 0;
    return gpio_state;
}
void MI_ESP_FLASH::OnChange(size_t old_index) {
    if (index == 0) {
        HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_RESET);
    }
    if (index == 1) {
        HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_SET);
    }
    _dbg0("%d index of ESP_RST_Pin", index);
}
// ----------------------------------------------------------------

using MenuContainer = WinMenuContainer<MI_RETURN, MI_ESP_FLASH, MI_ESP_RESET, MI_ESP_SYNC, MI_ESP_READ, MI_ESP_ERASE>;

class ScreenMenuESPUpdate : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("ESP UPDATE");

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    status_footer_t footer;

public:
    ScreenMenuESPUpdate();

protected:
};

ScreenMenuESPUpdate::ScreenMenuESPUpdate()
    : AddSuperWindow<screen_t>(nullptr, GuiDefaults::RectScreen)
    , menu(this, GuiDefaults::RectScreenBody, &container)
    , header(this)
    , footer(this) {
    header.SetText(_(label));
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list
}

ScreenFactory::UniquePtr GetScreenMenuESPUpdate() {
    return ScreenFactory::Screen<ScreenMenuESPUpdate>();
}
