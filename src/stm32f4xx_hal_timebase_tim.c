/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_hal_timebase_TIM.c
  * @brief   HAL time base based on the hardware TIM.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function configures the TIM6 as a time base source.
  *         The time source is configured  to have 1ms time base with a dedicated
  *         Tick interrupt priority.
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    RCC_ClkInitTypeDef clkconfig;
    uint32_t uwTimclock = 0;
    uint32_t uwPrescalerValue = 0;
    uint32_t pFLatency;

    /*Configure the TIM6 IRQ priority */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0);

    /* Enable the TIM6 global Interrupt */
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /* Enable TIM6 clock */
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    /* Compute TIM6 clock */
    uwTimclock = 2 * HAL_RCC_GetPCLK1Freq();

    /* Compute the prescaler value to have TIM6 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000) - 1);

    /* Initialize TIM6 */
    htim6.Instance = TIM6;

    /* Initialize TIMx peripheral as follow:
  + Period = [(TIM6CLK/1000) - 1]. to have a (1/1000) s time base.
  + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
  + ClockDivision = 0
  + Counter direction = Up
  */
    htim6.Init.Period = (1000000 / 1000) - 1;
    htim6.Init.Prescaler = uwPrescalerValue;
    htim6.Init.ClockDivision = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_Base_Init(&htim6) == HAL_OK) {
        /* Start the TIM time Base generation in interrupt mode */
        return HAL_TIM_Base_Start_IT(&htim6);
    }

    /* Return function status */
    return HAL_ERROR;
}

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM6 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_SuspendTick(void) {
    /* Disable TIM6 update Interrupt */
    __HAL_TIM_DISABLE_IT(&htim6, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM6 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_ResumeTick(void) {
    /* Enable TIM6 Update interrupt */
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
