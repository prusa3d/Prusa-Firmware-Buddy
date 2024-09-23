#pragma once
///
/// MCU Macros
///

#include <option/mcu.h>

#if MCU_IS_STM32F407VG()
    #define MCU_IS_STM32F4() 1
    #define MCU_IS_STM32G0() 0
#elif MCU_IS_STM32F427ZI()
    #define MCU_IS_STM32F4() 1
    #define MCU_IS_STM32G0() 0
#elif MCU_IS_STM32F429VI()
    #define MCU_IS_STM32F4() 1
    #define MCU_IS_STM32G0() 0
#elif MCU_IS_STM32G070RBT6()
    #define MCU_IS_STM32F4() 0
    #define MCU_IS_STM32G0() 1
#else
    #error Unknown MCU
#endif
