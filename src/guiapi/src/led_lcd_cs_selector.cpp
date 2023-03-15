/**
 * @file led_lcd_cs_selector.cpp
 * @author Radek Vana
 * @date 2021-08-23
 */

#include "led_lcd_cs_selector.hpp"
#include "main.h"
#include "hwio_pindef.h"
#include "gui_time.hpp" //gui::GetTick()

using namespace buddy::hw;

//this function is not in header, i prefer one extern over adding it to
//ili9488.hpp and including it
extern void ili9488_spi_wr_bytes(uint8_t *pb, uint16_t size);
extern void ili9488_cmd_nop(void);

uint32_t LED_LCD_CS_selector::last_tick = 0;

LED_LCD_CS_selector::LED_LCD_CS_selector(speed spd)
    : saved_prescaler(SPI_HANDLE_FOR(lcd).Init.BaudRatePrescaler) {
    if (HAL_SPI_DeInit(&SPI_HANDLE_FOR(lcd)) != HAL_OK) {
        Error_Handler();
    }
    uint32_t baudprescaler = 0;
    switch (spd) {
    case speed::MHz10_5:
        baudprescaler = SPI_BAUDRATEPRESCALER_8;
        break;
    case speed::MHz21:
        baudprescaler = SPI_BAUDRATEPRESCALER_4;
        break;
    case speed::MHz42:
        baudprescaler = SPI_BAUDRATEPRESCALER_2;
        break;
    }
    SPI_HANDLE_FOR(lcd).Init.BaudRatePrescaler = baudprescaler;
    if (HAL_SPI_Init(&SPI_HANDLE_FOR(lcd)) != HAL_OK) {
        Error_Handler();
    }

    ili9488_cmd_nop();
    displayCs.write(Pin::State::high);
    if (last_tick == gui::GetTick())
        osDelay(1); // not ready to send yet
}

LED_LCD_CS_selector::~LED_LCD_CS_selector() {
    displayCs.write(Pin::State::low);

    if (HAL_SPI_DeInit(&SPI_HANDLE_FOR(lcd)) != HAL_OK) {
        Error_Handler();
    }
    SPI_HANDLE_FOR(lcd).Init.BaudRatePrescaler = saved_prescaler;
    if (HAL_SPI_Init(&SPI_HANDLE_FOR(lcd)) != HAL_OK) {
        Error_Handler();
    }

    last_tick = gui::GetTick();
}

void LED_LCD_CS_selector::WrBytes(uint8_t *pb, uint16_t size) {
    ili9488_spi_wr_bytes(pb, size);
}
