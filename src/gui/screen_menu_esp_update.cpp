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

/* // ---------------------------------------------------------------- */
// // RESET
// class MI_ESP_RESET : public WI_LABEL_t {
//     constexpr static const char *const label = N_("ESP RESET");
//
// public:
//     MI_ESP_RESET()
//         : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
//     virtual void click(IWindowMenu & [>window_menu<]) override {
//
//         HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
//
//         // HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_SET);
//         // char at_cmd[] = "AT+RST\r\n";
//         // HAL_UART_Transmit(&huart6, (uint8_t *)at_cmd, sizeof(at_cmd), HAL_MAX_DELAY);
//
//         // HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_SET);
//         // osDelay(10);
//         // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_RESET);
//         // osDelay(10);
//         // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
//
//         _dbg0("HW RESET");
//     }
// };
/* // ---------------------------------------------------------------- */

// ----------------------------------------------------------------
// ESP UPLOADER - SYNC
class MI_ESP_SYNC : public WI_LABEL_t {
    constexpr static const char *const label = N_("ESP: sync");

public:
    MI_ESP_SYNC()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /* [ > window_menu < ] */) override {
        // if (lwesp_conn_upload_start(NULL, NULL, NULL, 1) == lwespOK) {
        if (lwesp_set_wifi_mode(LWESP_MODE_STA, NULL, NULL, 1) == lwespOK) {
            _dbg0("POSLANO SYNC");
        } else {
            _dbg0("SYNC ESP ERROR");
        }
    }
};
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// RESET - SWITCH
class MI_ESP_RESET : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("ESP RESET");
    constexpr static const char *const s_on = N_("On");
    constexpr static const char *const s_off = N_("Off");

    size_t init_index() const;

public:
    MI_ESP_RESET();

protected:
    virtual void OnChange(size_t) override;
};
MI_ESP_RESET::MI_ESP_RESET()
    : WI_SWITCH_t<2>(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no, _(s_on), _(s_off)) {}
size_t MI_ESP_RESET::init_index() const {
    return 0;
}
void MI_ESP_RESET::OnChange(size_t old_index) {
    if (index == 0) {
        // HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
    } else if (index == 1) {
        // HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
    }
    _dbg0("%d index of ESP_RST_Pin", index);
}
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
    return 0;
}
void MI_ESP_FLASH::OnChange(size_t old_index) {
    if (index == 0) {
        HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_RESET);
        // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
    } else if (index == 1) {
        HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_SET);
        // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_RESET);
    } else {
        // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
    }
    _dbg0("%d index of ESP_RST_Pin", index);
}
// ----------------------------------------------------------------

using MenuContainer = WinMenuContainer<MI_RETURN, MI_ESP_FLASH, MI_ESP_RESET, MI_ESP_SYNC>;

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
