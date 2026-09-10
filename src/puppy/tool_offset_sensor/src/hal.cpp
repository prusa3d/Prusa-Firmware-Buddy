#include "hal.hpp"
#include <bsod.h>
#include <device/peripherals.hpp>
#include <device/hal.h>
#include <i2c_manager.hpp>

extern "C" {
#include <FreeRTOSConfig.h>
}

static_assert(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY == 5);
#define ISR_PRIORITY_DEFAULT 5 // default ISR priority, used by ISRs that don't need specific ISR priority

#ifdef HASH_ALGOSELECTION_SHA256
    #error
#endif

// SVC_Handler + PendSV_Handler + SysTick_Handler are defined by FreeRTOS

void hal::panic() {
    NVIC_SystemReset();
}

void hal::reset() {
    NVIC_SystemReset();
}

void hal::set_status_led(bool set) {
    HAL_GPIO_WritePin(D_LED_USR_GPIO_Port, D_LED_USR_Pin, set ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// LDC1612 SD pin: LOW = enabled, HIGH = shutdown
void hal::ldc1612_set_enabled(bool enabled) {
    HAL_GPIO_WritePin(LDC1612_SD_GPIO_Port, LDC1612_SD_Pin, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
bool hal::ldc1612_is_enabled() {
    return HAL_GPIO_ReadPin(LDC1612_SD_GPIO_Port, LDC1612_SD_Pin) == GPIO_PIN_RESET;
}

namespace hal {
void init_clock();
void init_gpio();
void init_can();
void init_tim2();
void init_i2c();
} // namespace hal

namespace hal::peripherals {
TIM_HandleTypeDef htim2;
FDCAN_HandleTypeDef hfdcan1;
} // namespace hal::peripherals

void HAL_MspInit(void) {
    // This is important settings for clock configuration
    // It would loggically fit into init_clock, but STM CubeMX generated project
    // puts it here and we keep it here as well,
    // so it is easier to track possible future changes
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // On UFQFPN32, PA11/PA12 are remapped to PA9/PA10 by default.
    // Disable both remaps so PA9 (MCO) and PA12 (LDC1612_SD) work correctly.
    SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_PA11_RMP | SYSCFG_CFGR1_PA12_RMP);
}

void hal::init() {
    HAL_Init();
    hal::init_clock();
    hal::init_gpio();
    hal::init_can();
    hal::init_tim2();
    hal::init_i2c();
}

void hal::init_clock() {
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

    {
        /** Initializes the RCC Oscillators according to the specified parameters
         * in the RCC_OscInitTypeDef structure.
         */
        RCC_OscInitTypeDef RCC_OscInitStruct = {};
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
            bsod_system();
        }
    }

    {
        /** Initializes the CPU, AHB and APB buses clocks
         */
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
        RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
            bsod_system();
        }
    }

    {
        /** Initializes the common peripherals clocks
         */
        RCC_PeriphCLKInitTypeDef PeriphClkInit {};

        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_HSIKER;
        PeriphClkInit.HSIKerClockDivider = RCC_HSIKER_DIV2;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            bsod_system();
        }
    }
}

void hal::init_gpio() {
    // D_LED_USR - PA7
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = D_LED_USR_Pin,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(D_LED_USR_GPIO_Port, &GPIO_InitStruct);
    }
    // LDC1612_SD - PA12 (starts in shutdown, call ldc1612_set_enabled to wake)
    {
        LDC1612_SD_GPIO_CLK_ENABLE();
        HAL_GPIO_WritePin(LDC1612_SD_GPIO_Port, LDC1612_SD_Pin, GPIO_PIN_SET);
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = LDC1612_SD_Pin,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(LDC1612_SD_GPIO_Port, &GPIO_InitStruct);
    }
    // LDC1612_OSC - PA9
    {
        LDC1612_OSC_GPIO_CLK_ENABLE();
        HAL_RCC_MCOConfig(RCC_MCO1_PA9, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
    }
    // LDC1612_INTB - PA1 (falling edge interrupt, data ready)
    {
        LDC1612_INTB_GPIO_CLK_ENABLE();
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = LDC1612_INTB_Pin,
            .Mode = GPIO_MODE_IT_FALLING,
            .Pull = GPIO_PULLUP,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(LDC1612_INTB_GPIO_Port, &GPIO_InitStruct);
        HAL_NVIC_SetPriority(EXTI0_1_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
    }
}

void hal::init_can() {
    using namespace hal::peripherals;

    RCC_PeriphCLKInitTypeDef PeriphClkInit {};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN1;
    PeriphClkInit.Fdcan1ClockSelection = RCC_FDCAN1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        bsod_system();
    }

    __HAL_RCC_FDCAN1_CLK_ENABLE();

    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init = FDCAN_InitTypeDef {
        .ClockDivider = FDCAN_CLOCK_DIV1,
        .FrameFormat = enable_bit_rate_switch ? FDCAN_FRAME_FD_BRS : FDCAN_FRAME_FD_NO_BRS,
        .Mode = FDCAN_MODE_NORMAL,
        .AutoRetransmission = DISABLE,
        .TransmitPause = ENABLE,
        .ProtocolException = ENABLE,
        .NominalPrescaler = 2,
        .NominalSyncJumpWidth = 64,
        .NominalTimeSeg1 = 139,
        .NominalTimeSeg2 = 20,
        .DataPrescaler = 10,
        .DataSyncJumpWidth = 8,
        .DataTimeSeg1 = 27,
        .DataTimeSeg2 = 4,
        .StdFiltersNbr = 0,
        .ExtFiltersNbr = 0,
        .TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION,
    };

    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) {
        bsod_system();
    }

    /**FDCAN1 GPIO Configuration
      PB0      ------> FDCAN1_RX
      PB1      ------> FDCAN1_TX
      */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    static constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_0 | GPIO_PIN_1,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF3_FDCAN1,
    };
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* FDCAN1 interrupt Init */
    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);

    HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
}

void hal::init_tim2() {
    using namespace hal::peripherals;

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = HAL_RCC_GetHCLKFreq() - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        bsod_system();
    }

    HAL_NVIC_SetPriority(TIM2_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
        bsod_system();
    }
}

void hal::init_i2c() {
    __HAL_RCC_I2C1_CONFIG(RCC_I2C1CLKSOURCE_PCLK1);
    I2C1_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {
        .Pin = I2C1_SDA_Pin | I2C1_SCL_Pin,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = GPIO_AF6_I2C1,
    };
    HAL_GPIO_Init(I2C1_SDA_GPIO_Port, &GPIO_InitStruct);

    __HAL_RCC_I2C1_CLK_ENABLE();

    HAL_NVIC_SetPriority(I2C1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(I2C1_IRQn);

    i2c::Manager::get_instance(); // Ensure the I2C manager is initialized before we start using it
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == i2c::Manager::get_instance().handle()) {
        i2c::Manager::get_instance().isr_complete(i2c::Result::ok);
        return;
    }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == i2c::Manager::get_instance().handle()) {
        i2c::Manager::get_instance().isr_complete(i2c::Result::ok);
        return;
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == i2c::Manager::get_instance().handle()) {
        i2c::Manager::get_instance().isr_complete(i2c::Result::ok);
        return;
    }
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == i2c::Manager::get_instance().handle()) {
        i2c::Manager::get_instance().isr_complete(i2c::Result::ok);
        return;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == i2c::Manager::get_instance().handle()) {
        i2c::Manager::get_instance().isr_complete(i2c::Result::error);
        return;
    }
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c == i2c::Manager::get_instance().handle()) {
        i2c::Manager::get_instance().isr_complete(i2c::Result::error);
        return;
    }
}

LDC1612 hal::ldc1612 = {};
freertos::BinarySemaphore hal::ldc_data_ready;

extern "C" void hal_panic() {
    hal::panic();
}

extern "C" void NMI_Handler() {
    bsod_system();
}

extern "C" void HardFault_Handler() {
    bsod_system();
}

extern "C" void MemManage_Handler() {
    bsod_system();
}

extern "C" void BusFault_Handler() {
    bsod_system();
}

extern "C" void UsageFault_Handler() {
    bsod_system();
}

extern "C" void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&hal::peripherals::htim2);
}

extern "C" void FDCAN1_IT0_IRQHandler(void) {
    HAL_FDCAN_IRQHandler(&hal::peripherals::hfdcan1);
}

extern "C" void FDCAN1_IT1_IRQHandler(void) {
    HAL_FDCAN_IRQHandler(&hal::peripherals::hfdcan1);
}

extern "C" void EXTI0_1_IRQHandler() {
    if (__HAL_GPIO_EXTI_GET_IT(LDC1612_INTB_Pin)) {
        __HAL_GPIO_EXTI_CLEAR_IT(LDC1612_INTB_Pin);
        hal::ldc_data_ready.release_from_isr();
    }
}

extern "C" void I2C1_IRQHandler() {
    auto hi2c = i2c::Manager::get_instance().handle();
    if (hi2c->Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR)) {
        HAL_I2C_ER_IRQHandler(hi2c);
    } else {
        HAL_I2C_EV_IRQHandler(hi2c);
    }
}
