// sys.cpp - system functions
#include "sys.h"
#include "shared_config.h"
#include "stm32f4xx_hal.h"
#include "st25dv64k.h"
#include "dbg.h"

//firmware update flag
static const constexpr uint16_t FW_UPDATE_FLAG_ADDRESS = 0x040B;

extern SPI_HandleTypeDef hspi2;

//int sys_pll_freq = 100000000;
int sys_pll_freq = 168000000;

volatile data_exchange_t ram_data_exchange __attribute__((section(".boot_fw_data_exchange")));

version_t &boot_version = *(version_t *)(BOOTLOADER_VERSION_ADDRESS); // (address) from flash -> "volatile" is not necessary

volatile uint8_t *psys_fw_valid = (uint8_t *)0x080FFFFF; //last byte in the flash

void sys_reset(void) {
    static_assert(sizeof(data_exchange_t) == 16, "invalid sizeof(data_exchange_t)");

    uint32_t aircr = SCB->AIRCR & 0x0000ffff; //read AIRCR, mask VECTKEY
    if (__get_PRIMASK() & 1)
        __disable_irq(); //disable irq if enabled
    aircr |= 0x05fa0000; //set VECTKEY
    aircr |= 0x00000004; //set SYSRESETREQ
    SCB->AIRCR = aircr;  //write AIRCR
    while (1)
        ; //endless loop
}

void sys_dfu_boot(void) {
}

int sys_calc_flash_latency(int freq) {
    if (freq < 30000000)
        return 0;
    if (freq < 60000000)
        return 1;
    if (freq < 90000000)
        return 2;
    if (freq < 12000000)
        return 3;
    if (freq < 15000000)
        return 4;
    return 5;
}

int sys_pll_is_enabled(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    uint32_t FLatency;
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);              //read Osc config
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency); //read Clk config
    return ((RCC_OscInitStruct.PLL.PLLState == RCC_PLL_ON) && (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK)) ? 1 : 0;
}

void sys_pll_disable(void) {
    int irq = __get_PRIMASK() & 1;
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    uint32_t FLatency;
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);              //read Osc config
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency); //read Clk config
    if ((RCC_OscInitStruct.PLL.PLLState == RCC_PLL_OFF) && (RCC_ClkInitStruct.SYSCLKSource != RCC_SYSCLKSOURCE_PLLCLK))
        return;                                            //already disabled - exit
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;          //set PLL off
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE; //set CLK source HSE
    if (irq)
        __disable_irq();                                                        //disable irq while switching clock
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, sys_calc_flash_latency(HSE_VALUE)); //set Clk config first
    HAL_RCC_OscConfig(&RCC_OscInitStruct);                                      //set Osc config
    if (irq)
        __enable_irq();
    //HAL_RCC_GetOscConfig(&RCC_OscInitStruct);
    //HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency);
}

void sys_pll_enable(void) {
    int irq = __get_PRIMASK() & 1;
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /*
 	  RCC_OscInitTypeDef RCC_OscInitStruct1 = {0};
	  RCC_ClkInitTypeDef RCC_ClkInitStruct1 = {0};

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
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);              //read Osc config
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency); //read Clk config
    if ((RCC_OscInitStruct.PLL.PLLState == RCC_PLL_ON) && (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK))
        return;                                               //already enabled - exit
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;              //set PLL off
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; //set CLK source HSE
    if (irq)
        __disable_irq();                                                           //disable irq while switching clock
    HAL_RCC_OscConfig(&RCC_OscInitStruct);                                         //set Osc config first
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, sys_calc_flash_latency(sys_pll_freq)); //set Clk config
    if (irq)
        __enable_irq();
    //HAL_RCC_GetOscConfig(&RCC_OscInitStruct);
    //HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &FLatency);
}

int sys_sscg_is_enabled(void) {
    return (RCC->SSCGR & RCC_SSCGR_SSCGEN_Msk) ? 1 : 0;
}

void sys_sscg_disable(void) {
    uint32_t sscgr = RCC->SSCGR;
    __disable_irq();
    if ((sscgr & RCC_SSCGR_SSCGEN_Msk) == 0)
        return;
    sscgr &= ~((1 << RCC_SSCGR_SSCGEN_Pos) & RCC_SSCGR_SSCGEN_Msk);
    sys_pll_disable();
    RCC->SSCGR = sscgr;
    sys_pll_enable();
    _dbg0("written SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    sscgr = RCC->SSCGR;
    _dbg0("readback SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    __enable_irq();
}

void sys_sscg_enable(void) {
    uint32_t sscgr = RCC->SSCGR;
    uint32_t modper = ((sscgr & RCC_SSCGR_MODPER_Msk) >> RCC_SSCGR_MODPER_Pos);
    uint32_t incstep = ((sscgr & RCC_SSCGR_INCSTEP_Msk) >> RCC_SSCGR_INCSTEP_Pos);
    if (modper == 0)
        return;
    if (incstep == 0)
        return;
    sscgr |= (1 << RCC_SSCGR_SSCGEN_Pos) & RCC_SSCGR_SSCGEN_Msk;
    __disable_irq();
    sys_pll_disable();
    RCC->SSCGR = sscgr;
    sys_pll_enable();
    _dbg0("written SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    sscgr = RCC->SSCGR;
    _dbg0("readback SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    __enable_irq();
}

void sys_sscg_set_config(int freq, int depth) {
    int fpll = 2000000; //input pll frequency = 2MHz
    int plln;
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    uint32_t sscgr = RCC->SSCGR; //read current state of sscg
    uint32_t modper = ((sscgr & RCC_SSCGR_MODPER_Msk) >> RCC_SSCGR_MODPER_Pos);
    uint32_t incstep = ((sscgr & RCC_SSCGR_INCSTEP_Msk) >> RCC_SSCGR_INCSTEP_Pos);
    uint32_t spreadsel = ((sscgr & RCC_SSCGR_SPREADSEL_Msk) >> RCC_SSCGR_SPREADSEL_Pos);
    uint32_t sscgen = ((sscgr & RCC_SSCGR_SSCGEN_Msk) >> RCC_SSCGR_SSCGEN_Pos);
    _dbg0("SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    _dbg0(" MODPER    = 0x%08lx (%lu)", modper, modper);
    _dbg0(" INCSTEP   = 0x%08lx (%lu)", incstep, incstep);
    _dbg0(" SPREADSEL = 0x%08lx (%lu)", spreadsel, spreadsel);
    _dbg0(" SSCGEN    = 0x%08lx (%lu)", sscgen, sscgen);
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct); //read Osc config
    plln = RCC_OscInitStruct.PLL.PLLN;
    //modulation frequency = freq
    //MODPER = round [fPLL_IN / (4 x fMOD)]
    modper = (uint32_t)(((float)fpll / (4 * freq)) + 0.5F);
    //modulation depth = depth (+-depth%)
    //INCSTEP = round [(215 - 1) x md x PLLN) / (100 x 5 x MODPER)
    incstep = (uint32_t)((((float)214 * depth * plln) / (100 * 5 * modper)) + 0.5F);
    spreadsel = 0; //center spread
    sscgen = 1;    //spread spectrum modulation ENABLE
    sscgr = 0;
    sscgr |= (modper << RCC_SSCGR_MODPER_Pos) & RCC_SSCGR_MODPER_Msk;
    sscgr |= (incstep << RCC_SSCGR_INCSTEP_Pos) & RCC_SSCGR_INCSTEP_Msk;
    sscgr |= (spreadsel << RCC_SSCGR_SPREADSEL_Pos) & RCC_SSCGR_SPREADSEL_Msk;
    sscgr |= (sscgen << RCC_SSCGR_SSCGEN_Pos) & RCC_SSCGR_SSCGEN_Msk;
    sys_pll_disable();
    RCC->SSCGR = sscgr;
    sys_pll_enable();
    _dbg0("written SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    sscgr = RCC->SSCGR;
    modper = ((sscgr & RCC_SSCGR_MODPER_Msk) >> RCC_SSCGR_MODPER_Pos);
    incstep = ((sscgr & RCC_SSCGR_INCSTEP_Msk) >> RCC_SSCGR_INCSTEP_Pos);
    spreadsel = ((sscgr & RCC_SSCGR_SPREADSEL_Msk) >> RCC_SSCGR_SPREADSEL_Pos);
    sscgen = ((sscgr & RCC_SSCGR_SSCGEN_Msk) >> RCC_SSCGR_SSCGEN_Pos);
    _dbg0("readback SSCGR = 0x%08lx (%lu)", sscgr, sscgr);
    _dbg0(" MODPER    = 0x%08lx (%lu)", modper, modper);
    _dbg0(" INCSTEP   = 0x%08lx (%lu)", incstep, incstep);
    _dbg0(" SPREADSEL = 0x%08lx (%lu)", spreadsel, spreadsel);
    _dbg0(" SSCGEN    = 0x%08lx (%lu)", sscgen, sscgen);
}

int sys_sscg_get_config(float *pfreq, float *pdepth) {
    return 0;
}

uint32_t _spi_prescaler(int prescaler_num) {
    switch (prescaler_num) {
    case 0:
        return SPI_BAUDRATEPRESCALER_2; // 0x00000000U
    case 1:
        return SPI_BAUDRATEPRESCALER_4; // 0x00000008U
    case 2:
        return SPI_BAUDRATEPRESCALER_8; // 0x00000010U
    case 3:
        return SPI_BAUDRATEPRESCALER_16; // 0x00000018U
    case 4:
        return SPI_BAUDRATEPRESCALER_32; // 0x00000020U
    case 5:
        return SPI_BAUDRATEPRESCALER_64; // 0x00000028U
    case 6:
        return SPI_BAUDRATEPRESCALER_128; // 0x00000030U
    case 7:
        return SPI_BAUDRATEPRESCALER_256; // 0x00000038U
    }
    return SPI_BAUDRATEPRESCALER_2;
}

void sys_spi_set_prescaler(int prescaler_num) {
    int irq = __get_PRIMASK() & 1;
    if (irq)
        __disable_irq(); //disable irq while switching clock
    HAL_SPI_DeInit(&hspi2);
    hspi2.Init.BaudRatePrescaler = _spi_prescaler(prescaler_num);
    HAL_SPI_Init(&hspi2);
    if (irq)
        __enable_irq();
}

int sys_fw_update_is_enabled(void) {
    return (FW_UPDATE_ENABLE == st25dv64k_user_read(FW_UPDATE_FLAG_ADDRESS)) ? 1 : 0;
}

void sys_fw_update_enable(void) {
    st25dv64k_user_write(FW_UPDATE_FLAG_ADDRESS, FW_UPDATE_ENABLE);
}

void sys_fw_update_disable(void) {
    st25dv64k_user_write(FW_UPDATE_FLAG_ADDRESS, FW_UPDATE_DISABLE);
}
// firmware update on restart
int sys_fw_update_on_restart_is_enabled(void) {
    return (FW_UPDATE_ENABLE == ram_data_exchange.fw_update_flag) ? 1 : 0;
}

void sys_fw_update_on_restart_enable(void) {
    ram_data_exchange.fw_update_flag = FW_UPDATE_ENABLE;
}

extern void sys_fw_update_older_on_restart_enable(void) {
    ram_data_exchange.fw_update_flag = FW_UPDATE_OLDER;
}

void sys_fw_update_on_restart_disable(void) {
    ram_data_exchange.fw_update_flag = FW_UPDATE_DISABLE;
}

int sys_flash_is_empty(void *ptr, int size) {
    uint8_t *p = (uint8_t *)ptr;
    for (; size > 0; size--)
        if (*(p++) != 0xff)
            return 0;
    return 1;
}

int sys_flash_write(void *dst, void *src, int size) {
    uint8_t *pd = (uint8_t *)dst;
    uint8_t *ps = (uint8_t *)src;
    int i = 0;
    HAL_StatusTypeDef status;
    status = HAL_FLASH_Unlock();
    if (status == HAL_OK) {
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_BSY);
        for (; i < size; i++)
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)(pd++), *(ps++)) != HAL_OK)
                break;
    }
    HAL_FLASH_Lock();
    return i;
}

// erase single sector (0..11)
// sector  size  start       end
//  0      16kb  0x08000000  0x08003FFF
//  1      16kb  0x08004000  0x08007FFF
//  2      16kb  0x08008000  0x0800BFFF
//  3      16kb  0x0800c000  0x0800FFFF
//  4      64kb  0x08010000  0x0801FFFF
//  5     128kb  0x08020000  0x0803FFFF
//  6     128kb  0x08040000  0x0805FFFF
//  7     128kb  0x08060000  0x0807FFFF
//  8     128kb  0x08080000  0x0809FFFF
//  9     128kb  0x080A0000  0x080BFFFF
//  1     128kb  0x080C0000  0x080DFFFF
// 11     128kb  0x080E0000  0x080FFFFF
int sys_flash_erase_sector(unsigned int sector) {
    HAL_StatusTypeDef status;
    if ((sector < FLASH_SECTOR_0) || (sector > FLASH_SECTOR_11))
        return 0;
    status = HAL_FLASH_Unlock();
    if (status == HAL_OK) {
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
        FLASH_Erase_Sector(sector, VOLTAGE_RANGE_3);
    }
    HAL_FLASH_Lock();
    return (status == HAL_OK) ? 1 : 0;
}
