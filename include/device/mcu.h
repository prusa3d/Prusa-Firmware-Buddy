///
/// MCU Macros
///

#define STM32F407VG 1
#define STM32F429VI 3

#if MCU == STM32F407VG
    #define MCU_IS_STM32F4   1
    #define MCU_IS_STM32F40X 1
#elif MCU == STM32F429VI
    #define MCU_IS_STM32F4   1
    #define MCU_IS_STM32F42X 1
    #define MCU_IS_STM32F429 1
#else
    #error Unknown MCU
#endif
