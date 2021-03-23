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

extern UART_HandleTypeDef huart6;

class MI_ESP_FLASH_MODE : public WI_LABEL_t {
    constexpr static const char *const label = N_("Flash mode");

public:
    MI_ESP_FLASH_MODE()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {

        HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_RESET);
        char at_cmd[] = "AT+RST\r\n";
        HAL_UART_Transmit(&huart6, (uint8_t *)at_cmd, sizeof(at_cmd), HAL_MAX_DELAY);

        // HAL_GPIO_WritePin(GPIOE, ESP_GPIO0_Pin, GPIO_PIN_SET);
        // osDelay(10);
        // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_RESET);
        // osDelay(10);
        // HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);

        _dbg0("HW RESET with flash mode");

        // Eth::SaveMessage();
    }
};

using MenuContainer = WinMenuContainer<MI_RETURN, MI_ESP_FLASH_MODE>;

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
