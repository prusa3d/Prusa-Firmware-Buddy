/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2021 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the SystemView and RTT protocol, and J-Link.       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: 3.30                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SEGGER_SYSVIEW_Config_FreeRTOS.c
Purpose : Sample setup configuration of SystemView with FreeRTOS.
Revision: $Rev: 7745 $
*/
#include "FreeRTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "device/cmsis.h"

extern const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI;

/*********************************************************************
 *
 *       Defines, configurable
 *
 **********************************************************************
 */
// The application name to be displayed in SystemViewer
#define SYSVIEW_APP_NAME "Puppy Firmware"

// The target device name
#define SYSVIEW_DEVICE_NAME "Cortex-M0"

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_GET_TIMESTAMP in SEGGER_SYSVIEW_Conf.h
#define SYSVIEW_TIMESTAMP_FREQ (SYSTEM_CORE_CLOCK)

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#define SYSVIEW_CPU_FREQ SYSTEM_CORE_CLOCK

// The lowest RAM address used for IDs (pointers)
#define SYSVIEW_RAM_BASE (0x20000000)

/*********************************************************************
 *
 *       _cbSendSystemDesc()
 *
 *  Function description
 *    Sends SystemView description strings.
 */
static void _cbSendSystemDesc(void) {
    SEGGER_SYSVIEW_SendSysDesc("N=" SYSVIEW_APP_NAME ",D=" SYSVIEW_DEVICE_NAME ",O=FreeRTOS");
    SEGGER_SYSVIEW_SendSysDesc("I#15=SysTick");
    // Interrupt aliases
    // Uncomment those you want to see named in SystemView
    // Uncommenting all of them does not work, as it seems that SystemView limits the number of named ISRs.
    // SEGGER_SYSVIEW_SendSysDesc("I#16=WWDG");
    // SEGGER_SYSVIEW_SendSysDesc("I#17=PVD");
    // SEGGER_SYSVIEW_SendSysDesc("I#18=TAMP_STAMP");
    // SEGGER_SYSVIEW_SendSysDesc("I#19=RTC_WKUP");
    // SEGGER_SYSVIEW_SendSysDesc("I#20=FLASH");
    // SEGGER_SYSVIEW_SendSysDesc("I#21=RCC");
    // SEGGER_SYSVIEW_SendSysDesc("I#22=EXTI0");
    // SEGGER_SYSVIEW_SendSysDesc("I#23=EXTI1");
    // SEGGER_SYSVIEW_SendSysDesc("I#24=EXTI2");
    // SEGGER_SYSVIEW_SendSysDesc("I#25=EXTI3");
    // SEGGER_SYSVIEW_SendSysDesc("I#26=EXTI4");
    // SEGGER_SYSVIEW_SendSysDesc("I#27=DMA1_Stream0");
    SEGGER_SYSVIEW_SendSysDesc("I#28=DMA1_Stream1");
    // SEGGER_SYSVIEW_SendSysDesc("I#29=DMA1_Stream2");
    // SEGGER_SYSVIEW_SendSysDesc("I#30=DMA1_Stream3");
    // SEGGER_SYSVIEW_SendSysDesc("I#31=DMA1_Stream4");
    // SEGGER_SYSVIEW_SendSysDesc("I#32=DMA1_Stream5");
    // SEGGER_SYSVIEW_SendSysDesc("I#33=DMA1_Stream6");
    // SEGGER_SYSVIEW_SendSysDesc("I#34=ADC");
    // SEGGER_SYSVIEW_SendSysDesc("I#35=CAN1_TX");
    // SEGGER_SYSVIEW_SendSysDesc("I#36=CAN1_RX0");
    // SEGGER_SYSVIEW_SendSysDesc("I#37=CAN1_RX1");
    // SEGGER_SYSVIEW_SendSysDesc("I#38=CAN1_SCE");
    // SEGGER_SYSVIEW_SendSysDesc("I#39=EXTI9_5");
    // SEGGER_SYSVIEW_SendSysDesc("I#40=TIM1_BRK_TIM9");
    // SEGGER_SYSVIEW_SendSysDesc("I#41=TIM1_UP_TIM10");
    // SEGGER_SYSVIEW_SendSysDesc("I#42=TIM1_TRG_COM_TIM11");
    // SEGGER_SYSVIEW_SendSysDesc("I#43=TIM1_CC");
    SEGGER_SYSVIEW_SendSysDesc("I#44=TIM2");
    // SEGGER_SYSVIEW_SendSysDesc("I#45=TIM3");
    // SEGGER_SYSVIEW_SendSysDesc("I#46=TIM4");
    // SEGGER_SYSVIEW_SendSysDesc("I#47=I2C1_EV");
    // SEGGER_SYSVIEW_SendSysDesc("I#48=I2C1_ER");
    // SEGGER_SYSVIEW_SendSysDesc("I#49=I2C2_EV");
    // SEGGER_SYSVIEW_SendSysDesc("I#50=I2C2_ER");
    // SEGGER_SYSVIEW_SendSysDesc("I#51=SPI1");
    // SEGGER_SYSVIEW_SendSysDesc("I#52=SPI2");
    // SEGGER_SYSVIEW_SendSysDesc("I#53=USART1");
    // SEGGER_SYSVIEW_SendSysDesc("I#54=USART2");
    // SEGGER_SYSVIEW_SendSysDesc("I#55=USART3");
    // SEGGER_SYSVIEW_SendSysDesc("I#56=EXTI15_10");
    // SEGGER_SYSVIEW_SendSysDesc("I#57=RTC_Alarm");
    // SEGGER_SYSVIEW_SendSysDesc("I#58=OTG_FS_WKUP");
    // SEGGER_SYSVIEW_SendSysDesc("I#59=TIM8_BRK_TIM12");
    // SEGGER_SYSVIEW_SendSysDesc("I#60=TIM8_UP_TIM13");
    // SEGGER_SYSVIEW_SendSysDesc("I#61=TIM8_TRG_COM_TIM14");
    // SEGGER_SYSVIEW_SendSysDesc("I#62=TIM8_CC");
    // SEGGER_SYSVIEW_SendSysDesc("I#63=DMA1_Stream7");
    // SEGGER_SYSVIEW_SendSysDesc("I#64=FSMC");
    // SEGGER_SYSVIEW_SendSysDesc("I#65=SDIO");
    // SEGGER_SYSVIEW_SendSysDesc("I#66=TIM5");
    // SEGGER_SYSVIEW_SendSysDesc("I#67=SPI3");
    // SEGGER_SYSVIEW_SendSysDesc("I#68=UART4");
    // SEGGER_SYSVIEW_SendSysDesc("I#69=UART5");
    // SEGGER_SYSVIEW_SendSysDesc("I#70=TIM6_DAC");
    // SEGGER_SYSVIEW_SendSysDesc("I#71=TIM7");
    // SEGGER_SYSVIEW_SendSysDesc("I#72=DMA2_Stream0");
    // SEGGER_SYSVIEW_SendSysDesc("I#73=DMA2_Stream1");
    // SEGGER_SYSVIEW_SendSysDesc("I#74=DMA2_Stream2");
    // SEGGER_SYSVIEW_SendSysDesc("I#75=DMA2_Stream3");
    // SEGGER_SYSVIEW_SendSysDesc("I#76=DMA2_Stream4");
    // SEGGER_SYSVIEW_SendSysDesc("I#77=ETH");
    // SEGGER_SYSVIEW_SendSysDesc("I#78=ETH_WKUP");
    // SEGGER_SYSVIEW_SendSysDesc("I#79=CAN2_TX");
    // SEGGER_SYSVIEW_SendSysDesc("I#80=CAN2_RX0");
    // SEGGER_SYSVIEW_SendSysDesc("I#81=CAN2_RX1");
    // SEGGER_SYSVIEW_SendSysDesc("I#82=CAN2_SCE");
    // SEGGER_SYSVIEW_SendSysDesc("I#83=OTG_FS");
    // SEGGER_SYSVIEW_SendSysDesc("I#84=DMA2_Stream5");
    // SEGGER_SYSVIEW_SendSysDesc("I#85=DMA2_Stream6");
    // SEGGER_SYSVIEW_SendSysDesc("I#86=DMA2_Stream7");
    // SEGGER_SYSVIEW_SendSysDesc("I#87=USART6");
    // SEGGER_SYSVIEW_SendSysDesc("I#88=I2C3_EV");
    // SEGGER_SYSVIEW_SendSysDesc("I#89=I2C3_ER");
    // SEGGER_SYSVIEW_SendSysDesc("I#90=OTG_HS_EP1_OUT");
    // SEGGER_SYSVIEW_SendSysDesc("I#91=OTG_HS_EP1_IN");
    // SEGGER_SYSVIEW_SendSysDesc("I#92=OTG_HS_WKUP");
    // SEGGER_SYSVIEW_SendSysDesc("I#93=OTG_HS");
    // SEGGER_SYSVIEW_SendSysDesc("I#94=DCMI");
    // SEGGER_SYSVIEW_SendSysDesc("I#95=CRYPTO");
    // SEGGER_SYSVIEW_SendSysDesc("I#96=HASH_RNG");
    // SEGGER_SYSVIEW_SendSysDesc("I#97=FPU");*/
}

/*********************************************************************
 *
 *       Global functions
 *
 **********************************************************************
 */
void SEGGER_SYSVIEW_Conf(void) {
    SEGGER_SYSVIEW_Init(SYSVIEW_TIMESTAMP_FREQ, SYSVIEW_CPU_FREQ,
        &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);
    SEGGER_SYSVIEW_SetRAMBase(SYSVIEW_RAM_BASE);
}

/*************************** End of file ****************************/
