#include "interrupts_helper.hpp"
#include <common/bsod.h>
#include <tusb.h>
#include <wdt.hpp>

extern ETH_HandleTypeDef heth;
extern HCD_HandleTypeDef hhcd_USB_OTG_HS;

extern "C" void MemManage_Handler() {
    bsod("MemManage_Handler");
}

extern "C" void BusFault_Handler() {
    bsod("BusFault_Handler");
}

extern "C" void UsageFault_Handler() {
    bsod("UsageFault_Handler");
}

extern "C" void DebugMon_Handler() {
}

TRACED_ISR(TIM8_TRG_COM_TIM14_IRQHandler, HAL_TIM_IRQHandler, &htim14);
TRACED_ISR(WWDG_IRQHandler, HAL_WWDG_IRQHandler, &hwwdg);
TRACED_ISR(OTG_FS_IRQHandler, tud_int_handler, 0);

#if !BOARD_IS_XL_DEV_KIT_XLB()

TRACED_ISR(OTG_HS_IRQHandler, HAL_HCD_IRQHandler, &hhcd_USB_OTG_HS);
TRACED_ISR(ETH_IRQHandler, HAL_ETH_IRQHandler, &heth);

#endif
