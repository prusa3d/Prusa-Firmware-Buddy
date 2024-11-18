// sys.cpp - system functions
#include <stdlib.h>
#include "sys.h"
#include "shared_config.h"
#include "stm32f4xx_hal.h"
#include "st25dv64k.h"
#include <buddy/main.h>
#include <logging/log.hpp>
#include "disable_interrupts.h"
#include "utility_extensions.hpp"
#include <string.h>

LOG_COMPONENT_REF(Buddy);

#define DFU_REQUEST_RTC_BKP_REGISTER RTC->BKP0R

// magic value of RTC->BKP0R for requesting DFU bootloader entry
static const constexpr uint32_t DFU_REQUESTED_MAGIC_VALUE = 0xF1E2D3C5;

// firmware update flag
static const constexpr uint16_t FW_UPDATE_FLAG_ADDRESS = 0x040B;

// int sys_pll_freq = 100000000;
int sys_pll_freq = 168000000;

version_t &boot_version = *(version_t *)(BOOTLOADER_VERSION_ADDRESS); // (address) from flash -> "volatile" is not necessary

volatile uint8_t *psys_fw_valid = (uint8_t *)0x080FFFFF; // last byte in the flash

// Needs to be RAM function as it is called when erasing the flash
void __RAM_FUNC sys_reset(void) {
    uint32_t aircr = SCB->AIRCR & 0x0000ffff; // read AIRCR, mask VECTKEY
    __disable_irq();
    aircr |= 0x05fa0000; // set VECTKEY
    aircr |= 0x00000004; // set SYSRESETREQ
    SCB->AIRCR = aircr; // write AIRCR
    while (1)
        ; // endless loop
}

void sys_dfu_request_and_reset(void) {
    DFU_REQUEST_RTC_BKP_REGISTER = DFU_REQUESTED_MAGIC_VALUE;
    NVIC_SystemReset();
}

bool sys_dfu_requested(void) {
    return DFU_REQUEST_RTC_BKP_REGISTER == DFU_REQUESTED_MAGIC_VALUE;
}

void sys_dfu_boot_enter(void) {
    // clear the flag
    DFU_REQUEST_RTC_BKP_REGISTER = 0;

    // disable systick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // remap memory
    SYSCFG->MEMRMP = 0x01;

    // enter the bootloader
    volatile uintptr_t system_addr_start = 0x1FFF0000;
    auto system_bootloader_start = (void (*)(void))(*(uint32_t *)(system_addr_start + 4));
    __set_MSP(*(uint32_t *)system_addr_start); // prepare stack pointer
    system_bootloader_start(); // jump into the bootloader

    // we should never reach this
    abort();
}

int sys_calc_flash_latency(int freq) {
    if (freq < 30000000) {
        return 0;
    }
    if (freq < 60000000) {
        return 1;
    }
    if (freq < 90000000) {
        return 2;
    }
    if (freq < 12000000) {
        return 3;
    }
    if (freq < 15000000) {
        return 4;
    }
    return 5;
}

int sys_pll_is_enabled(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct {};
    uint32_t FLatency;
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct); // read Osc config
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency); // read Clk config
    return ((RCC_OscInitStruct.PLL.PLLState == RCC_PLL_ON) && (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK)) ? 1 : 0;
}

void sys_pll_disable(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct {};
    uint32_t FLatency;
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct); // read Osc config
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency); // read Clk config
    if ((RCC_OscInitStruct.PLL.PLLState == RCC_PLL_OFF) && (RCC_ClkInitStruct.SYSCLKSource != RCC_SYSCLKSOURCE_PLLCLK)) {
        return; // already disabled - exit
    }
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF; // set PLL off
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE; // set CLK source HSE

    buddy::DisableInterrupts disable_interrupts;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, sys_calc_flash_latency(HSE_VALUE)); // set Clk config first
    HAL_RCC_OscConfig(&RCC_OscInitStruct); // set Osc config
}

void sys_pll_enable(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct {};

    /*
          RCC_OscInitTypeDef RCC_OscInitStruct1 {};
          RCC_ClkInitTypeDef RCC_ClkInitStruct1 {};

          RCC_OscInitStruct1.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
          RCC_OscInitStruct1.HSEState = RCC_HSE_ON;
          RCC_OscInitStruct1.LSIState = RCC_LSI_ON;
          RCC_OscInitStruct1.PLL.PLLState = RCC_PLL_ON;
          RCC_OscInitStruct1.PLL.PLLSource = RCC_PLLSOURCE_HSE;
          RCC_OscInitStruct1.PLL.PLLM = 6;
          RCC_OscInitStruct1.PLL.PLLN = 100;
          RCC_OscInitStruct1.PLL.PLLP = RCC_PLLP_DIV2;
          RCC_OscInitStruct1.PLL.PLLQ = 7;
          RCC_ClkInitStruct1.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                      |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
          RCC_ClkInitStruct1.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
          RCC_ClkInitStruct1.AHBCLKDivider = RCC_SYSCLK_DIV1;
          RCC_ClkInitStruct1.APB1CLKDivider = RCC_HCLK_DIV4;
          RCC_ClkInitStruct1.APB2CLKDivider = RCC_HCLK_DIV2;
*/

    uint32_t FLatency;
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct); // read Osc config
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency); // read Clk config
    if ((RCC_OscInitStruct.PLL.PLLState == RCC_PLL_ON) && (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK)) {
        return; // already enabled - exit
    }
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; // set PLL off
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // set CLK source HSE

    buddy::DisableInterrupts disable_interrupts;
    HAL_RCC_OscConfig(&RCC_OscInitStruct); // set Osc config first
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, sys_calc_flash_latency(sys_pll_freq)); // set Clk config
}

int sys_fw_update_is_enabled(void) {
    return (ftrstd::to_underlying(FwAutoUpdate::on) == st25dv64k_user_read(FW_UPDATE_FLAG_ADDRESS)) ? 1 : 0;
}

void sys_fw_update_enable(void) {
    st25dv64k_user_write(FW_UPDATE_FLAG_ADDRESS, ftrstd::to_underlying(FwAutoUpdate::on));
}

void sys_fw_update_disable(void) {
    st25dv64k_user_write(FW_UPDATE_FLAG_ADDRESS, ftrstd::to_underlying(FwAutoUpdate::off));
}

bool version_less_than(const version_t *a, const uint8_t major, const uint8_t minor, const uint8_t patch) {
    if (a->major != major) {
        return a->major < major;
    }
    if (a->minor != minor) {
        return a->minor < minor;
    }
    return a->patch < patch;
}
