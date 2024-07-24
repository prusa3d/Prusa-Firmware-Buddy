#include "PuppyConfig.hpp"
#include "hal/HAL_PWM.hpp"
#include "hal/HAL_System.hpp"
#include "hal/HAL_Common.hpp"
#include "hal/HAL_MultiWatchdog.hpp"

#include "stm32g0xx_hal.h"
#include <cstring>
#include <cstdlib>
#include <device/board.h>
#include "buddy/priorities_config.h"

#define DISABLE_HB_0  0x00000001
#define DISABLE_HB_1  0x00000002
#define DISABLE_HB_2  0x00000004
#define DISABLE_HB_3  0x00000008
#define DISABLE_HB_4  0x00000010
#define DISABLE_HB_5  0x00000020
#define DISABLE_HB_6  0x00000040
#define DISABLE_HB_7  0x00000080
#define DISABLE_HB_8  0x00000100
#define DISABLE_HB_9  0x00000200
#define DISABLE_HB_10 0x00000400
#define DISABLE_HB_11 0x00000800
#define DISABLE_HB_12 0x00001000
#define DISABLE_HB_13 0x00002000
#define DISABLE_HB_14 0x00004000
#define DISABLE_HB_15 0x00008000

#define ENABLE_HB_0  0x00010000
#define ENABLE_HB_1  0x00020000
#define ENABLE_HB_2  0x00040000
#define ENABLE_HB_3  0x00080000
#define ENABLE_HB_4  0x00100000
#define ENABLE_HB_5  0x00200000
#define ENABLE_HB_6  0x00400000
#define ENABLE_HB_7  0x00800000
#define ENABLE_HB_8  0x01000000
#define ENABLE_HB_9  0x02000000
#define ENABLE_HB_10 0x04000000
#define ENABLE_HB_11 0x08000000
#define ENABLE_HB_12 0x10000000
#define ENABLE_HB_13 0x20000000
#define ENABLE_HB_14 0x40000000
#define ENABLE_HB_15 0x80000000

#define GPIO_HB_0 GPIOB
#define GPIO_HB_1 GPIOB
#define GPIO_HB_2 GPIOC
#define GPIO_HB_3 GPIOB
#define GPIO_HB_4 GPIOD
#define GPIO_HB_5 GPIOD
#define GPIO_HB_6 GPIOD
#define GPIO_HB_7 GPIOC

#define GPIO_HB_8  GPIOB
#define GPIO_HB_9  GPIOA
#define GPIO_HB_10 GPIOB
#define GPIO_HB_11 GPIOB
#define GPIO_HB_12 GPIOC
#define GPIO_HB_13 GPIOA
#define GPIO_HB_14 GPIOC
#define GPIO_HB_15 GPIOC

#define PIN_HB_0 GPIO_PIN_4
#define PIN_HB_1 GPIO_PIN_5
#define PIN_HB_2 GPIO_PIN_8
#define PIN_HB_3 GPIO_PIN_9
#define PIN_HB_4 GPIO_PIN_1
#define PIN_HB_5 GPIO_PIN_2
#define PIN_HB_6 GPIO_PIN_0
#define PIN_HB_7 GPIO_PIN_9

#define PIN_HB_8  GPIO_PIN_13
#define PIN_HB_9  GPIO_PIN_8
#define PIN_HB_10 GPIO_PIN_15
#define PIN_HB_11 GPIO_PIN_14
#define PIN_HB_12 GPIO_PIN_13
#define PIN_HB_13 GPIO_PIN_11
#define PIN_HB_14 GPIO_PIN_15
#define PIN_HB_15 GPIO_PIN_14

namespace hal::PWMDriver {

static TIM_HandleTypeDef s_htim;

static uint32_t s_PWMPulseCounter = 0;
static uint32_t s_EdgeMaskList_A[PWM_PERIOD_LENGTH];
static uint32_t s_EdgeMaskList_B[PWM_PERIOD_LENGTH];
static uint32_t *s_pActualEdgeMaskList = s_EdgeMaskList_A;

static uint16_t s_PulseMaskList[PWM_PERIOD_LENGTH];
// uint32_t s_PulseCounts[PWM_PERIOD_LENGTH];

extern "C" {
void TIM3_IRQHandler(void);
}

static void InitGPIOPin(GPIO_TypeDef *GPIOx, uint32_t pin, GPIO_InitTypeDef *GPIO_Init) {
    GPIO_Init->Pin = pin;
    HAL_GPIO_Init(GPIOx, GPIO_Init);
}

bool Init() {
    // init data structures
    memset(s_EdgeMaskList_A, 0, sizeof(s_EdgeMaskList_A));
    memset(s_EdgeMaskList_B, 0, sizeof(s_EdgeMaskList_B));
    memset(s_PulseMaskList, 0, sizeof(s_PulseMaskList));
    //    memset(s_PulseCounts, 0, sizeof(s_PulseCounts));

    // GPIO clock enable
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // init GPIO output pins
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;

    InitGPIOPin(GPIO_HB_0, PIN_HB_0, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_1, PIN_HB_1, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_2, PIN_HB_2, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_3, PIN_HB_3, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_4, PIN_HB_4, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_5, PIN_HB_5, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_6, PIN_HB_6, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_7, PIN_HB_7, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_8, PIN_HB_8, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_9, PIN_HB_9, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_10, PIN_HB_10, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_11, PIN_HB_11, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_12, PIN_HB_12, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_13, PIN_HB_13, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_14, PIN_HB_14, &GPIO_InitStruct);
    InitGPIOPin(GPIO_HB_15, PIN_HB_15, &GPIO_InitStruct);

    // TIM3 timer
    HAL_NVIC_SetPriority(TIM3_IRQn, ISR_PRIORITY_PWM_TIMER, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    __HAL_RCC_TIM3_CLK_ENABLE();

    uint32_t uwTimclock = HAL_RCC_GetPCLK1Freq();
    uint32_t uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U); // TIM3 counter clock equal to 1MHz

    s_htim.Instance = TIM3;
    s_htim.Init.Period = (1000000U / PWM_TIMER_FREQUENCY) - 1U;
    s_htim.Init.Prescaler = uwPrescalerValue;
    s_htim.Init.ClockDivision = 0;
    s_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&s_htim) != HAL_OK) {
        return false;
    }

    if (HAL_TIM_Base_Start_IT(&s_htim) != HAL_OK) {
        return false;
    }

    return true;
}

void AddPWMPulse(uint32_t heatbedletIndex, uint32_t pulseStartEdge, uint32_t pulseLength) {
    if (pulseLength > 0) {
        uint32_t hbMask = 1 << heatbedletIndex;
        uint32_t pulseEndEdge = pulseStartEdge + pulseLength;
        if (pulseEndEdge >= PWM_PERIOD_LENGTH) {
            pulseEndEdge -= PWM_PERIOD_LENGTH;
        }

        if (pulseStartEdge != pulseEndEdge) {
            if (pulseEndEdge >= PWM_PERIOD_LENGTH) {
                bsod("PWM pulse is too long!");
            }
            uint32_t *pEdgeMaskList = (s_pActualEdgeMaskList == s_EdgeMaskList_A) ? s_EdgeMaskList_B : s_EdgeMaskList_A;
            pEdgeMaskList[pulseStartEdge] |= hbMask << 16;
            pEdgeMaskList[pulseEndEdge] |= hbMask;
        }

        for (uint32_t i = 0; i < pulseLength; i++) {
            uint32_t idx = pulseStartEdge + i;
            if (idx >= PWM_PERIOD_LENGTH) {
                idx -= PWM_PERIOD_LENGTH;
            }

            s_PulseMaskList[idx] |= hbMask;
            //            s_PulseCounts[idx]++;
        }
    }
}

void ApplyPWMPattern() {
    // switch edge lists
    // transition should be as smooth as possible
    uint32_t *pEdgeMaskList = (s_pActualEdgeMaskList == s_EdgeMaskList_A) ? s_EdgeMaskList_B : s_EdgeMaskList_A;

    CRITICAL_SECTION_START;

    s_pActualEdgeMaskList = pEdgeMaskList;
    uint32_t pulseIndex = s_PWMPulseCounter % PWM_PERIOD_LENGTH;
    uint32_t pulseMask = s_PulseMaskList[pulseIndex];
    uint32_t edgeMask = pulseMask << 16 | (~pulseMask & 0xFFFF);
    uint32_t oldEdgeMask = s_pActualEdgeMaskList[pulseIndex];
    s_pActualEdgeMaskList[pulseIndex] = edgeMask;
    TIM3_IRQHandler();
    s_pActualEdgeMaskList[pulseIndex] = oldEdgeMask;

    CRITICAL_SECTION_END;

    // clear buffers an prepare them for next pwm pattern
    pEdgeMaskList = (s_pActualEdgeMaskList == s_EdgeMaskList_A) ? s_EdgeMaskList_B : s_EdgeMaskList_A;
    memset(pEdgeMaskList, 0, sizeof(s_EdgeMaskList_A));
    memset(s_PulseMaskList, 0, sizeof(s_PulseMaskList));
    //    memset(s_PulseCounts, 0, sizeof(s_PulseCounts));
}

void TurnOffAll() {
    memset(s_pActualEdgeMaskList, 0, sizeof(s_EdgeMaskList_A));
    memset(s_PulseMaskList, 0, sizeof(s_PulseMaskList));

    GPIO_HB_0->BRR = PIN_HB_0;
    GPIO_HB_1->BRR = PIN_HB_1;
    GPIO_HB_2->BRR = PIN_HB_2;
    GPIO_HB_3->BRR = PIN_HB_3;
    GPIO_HB_4->BRR = PIN_HB_4;
    GPIO_HB_5->BRR = PIN_HB_5;
    GPIO_HB_6->BRR = PIN_HB_6;
    GPIO_HB_7->BRR = PIN_HB_7;

    GPIO_HB_8->BRR = PIN_HB_8;
    GPIO_HB_9->BRR = PIN_HB_9;
    GPIO_HB_10->BRR = PIN_HB_10;
    GPIO_HB_11->BRR = PIN_HB_11;
    GPIO_HB_12->BRR = PIN_HB_12;
    GPIO_HB_13->BRR = PIN_HB_13;
    GPIO_HB_14->BRR = PIN_HB_14;
    GPIO_HB_15->BRR = PIN_HB_15;
}

static hal::MultiWatchdog pwm_wdg; // Add one instance of watchdog

void TIM3_IRQHandler() {
    // this routine shall be as fast as possible, because it is called very often
    // see configuration constant PWM_TIMER_FREQUENCY
    // measured average duration on STM32G0@56MHz is 1.62 microseconds in debug configuration

    CLEAR_BIT(s_htim.Instance->SR, TIM_SR_UIF);

    uint32_t pulseMask = s_pActualEdgeMaskList[(s_PWMPulseCounter++) % PWM_PERIOD_LENGTH];

    if (pulseMask == 0) {
        return;
    }

    if (pulseMask & (DISABLE_HB_0 | DISABLE_HB_1 | DISABLE_HB_2 | DISABLE_HB_3 | DISABLE_HB_4 | DISABLE_HB_5 | DISABLE_HB_6 | DISABLE_HB_7 | DISABLE_HB_8 | DISABLE_HB_9 | DISABLE_HB_10 | DISABLE_HB_11 | DISABLE_HB_12 | DISABLE_HB_13 | DISABLE_HB_14 | DISABLE_HB_15)) {
        if (pulseMask & (DISABLE_HB_0 | DISABLE_HB_1 | DISABLE_HB_2 | DISABLE_HB_3)) {
            if (pulseMask & DISABLE_HB_0) {
                GPIO_HB_0->BRR = PIN_HB_0;
            }
            if (pulseMask & DISABLE_HB_1) {
                GPIO_HB_1->BRR = PIN_HB_1;
            }
            if (pulseMask & DISABLE_HB_2) {
                GPIO_HB_2->BRR = PIN_HB_2;
            }
            if (pulseMask & DISABLE_HB_3) {
                GPIO_HB_3->BRR = PIN_HB_3;
            }
        }
        if (pulseMask & (DISABLE_HB_4 | DISABLE_HB_5 | DISABLE_HB_6 | DISABLE_HB_7)) {
            if (pulseMask & DISABLE_HB_4) {
                GPIO_HB_4->BRR = PIN_HB_4;
            }
            if (pulseMask & DISABLE_HB_5) {
                GPIO_HB_5->BRR = PIN_HB_5;
            }
            if (pulseMask & DISABLE_HB_6) {
                GPIO_HB_6->BRR = PIN_HB_6;
            }
            if (pulseMask & DISABLE_HB_7) {
                GPIO_HB_7->BRR = PIN_HB_7;
            }
        }
        if (pulseMask & (DISABLE_HB_8 | DISABLE_HB_9 | DISABLE_HB_10 | DISABLE_HB_11)) {
            if (pulseMask & DISABLE_HB_8) {
                GPIO_HB_8->BRR = PIN_HB_8;
            }
            if (pulseMask & DISABLE_HB_9) {
                GPIO_HB_9->BRR = PIN_HB_9;
            }
            if (pulseMask & DISABLE_HB_10) {
                GPIO_HB_10->BRR = PIN_HB_10;
            }
            if (pulseMask & DISABLE_HB_11) {
                GPIO_HB_11->BRR = PIN_HB_11;
            }
        }
        if (pulseMask & (DISABLE_HB_12 | DISABLE_HB_13 | DISABLE_HB_14 | DISABLE_HB_15)) {
            if (pulseMask & DISABLE_HB_12) {
                GPIO_HB_12->BRR = PIN_HB_12;
            }
            if (pulseMask & DISABLE_HB_13) {
                GPIO_HB_13->BRR = PIN_HB_13;
            }
            if (pulseMask & DISABLE_HB_14) {
                GPIO_HB_14->BRR = PIN_HB_14;
            }
            if (pulseMask & DISABLE_HB_15) {
                GPIO_HB_15->BRR = PIN_HB_15;
            }
        }
    }

    if (pulseMask & (ENABLE_HB_0 | ENABLE_HB_1 | ENABLE_HB_2 | ENABLE_HB_3 | ENABLE_HB_4 | ENABLE_HB_5 | ENABLE_HB_6 | ENABLE_HB_7 | ENABLE_HB_8 | ENABLE_HB_9 | ENABLE_HB_10 | ENABLE_HB_11 | ENABLE_HB_12 | ENABLE_HB_13 | ENABLE_HB_14 | ENABLE_HB_15)) {
        if (pulseMask & (ENABLE_HB_0 | ENABLE_HB_1 | ENABLE_HB_2 | ENABLE_HB_3)) {
            if (pulseMask & ENABLE_HB_0) {
                GPIO_HB_0->BSRR = PIN_HB_0;
            }
            if (pulseMask & ENABLE_HB_1) {
                GPIO_HB_1->BSRR = PIN_HB_1;
            }
            if (pulseMask & ENABLE_HB_2) {
                GPIO_HB_2->BSRR = PIN_HB_2;
            }
            if (pulseMask & ENABLE_HB_3) {
                GPIO_HB_3->BSRR = PIN_HB_3;
            }
        }
        if (pulseMask & (ENABLE_HB_4 | ENABLE_HB_5 | ENABLE_HB_6 | ENABLE_HB_7)) {
            if (pulseMask & ENABLE_HB_4) {
                GPIO_HB_4->BSRR = PIN_HB_4;
            }
            if (pulseMask & ENABLE_HB_5) {
                GPIO_HB_5->BSRR = PIN_HB_5;
            }
            if (pulseMask & ENABLE_HB_6) {
                GPIO_HB_6->BSRR = PIN_HB_6;
            }
            if (pulseMask & ENABLE_HB_7) {
                GPIO_HB_7->BSRR = PIN_HB_7;
            }
        }
        if (pulseMask & (ENABLE_HB_8 | ENABLE_HB_9 | ENABLE_HB_10 | ENABLE_HB_11)) {
            if (pulseMask & ENABLE_HB_8) {
                GPIO_HB_8->BSRR = PIN_HB_8;
            }
            if (pulseMask & ENABLE_HB_9) {
                GPIO_HB_9->BSRR = PIN_HB_9;
            }
            if (pulseMask & ENABLE_HB_10) {
                GPIO_HB_10->BSRR = PIN_HB_10;
            }
            if (pulseMask & ENABLE_HB_11) {
                GPIO_HB_11->BSRR = PIN_HB_11;
            }
        }
        if (pulseMask & (ENABLE_HB_12 | ENABLE_HB_13 | ENABLE_HB_14 | ENABLE_HB_15)) {
            if (pulseMask & ENABLE_HB_12) {
                GPIO_HB_12->BSRR = PIN_HB_12;
            }
            if (pulseMask & ENABLE_HB_13) {
                GPIO_HB_13->BSRR = PIN_HB_13;
            }
            if (pulseMask & ENABLE_HB_14) {
                GPIO_HB_14->BSRR = PIN_HB_14;
            }
            if (pulseMask & ENABLE_HB_15) {
                GPIO_HB_15->BSRR = PIN_HB_15;
            }
        }
    }

    pwm_wdg.kick(false);
}

} // namespace hal::PWMDriver
