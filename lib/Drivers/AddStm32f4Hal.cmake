add_library(
  STM32F4_HAL
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
  STM32F4xx_HAL_Driver/Src/Legacy/stm32f4xx_hal_eth.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_hcd.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_wwdg.c
  STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c
  )

target_include_directories(STM32F4_HAL PUBLIC STM32F4xx_HAL_Driver/Inc)
target_include_directories(STM32F4_HAL PUBLIC STM32F4xx_HAL_Driver/Inc/Legacy)
target_link_libraries(STM32F4_HAL PUBLIC STM32F4_HAL_Config STM32F4xx::CMSIS)

add_library(STM32F4::HAL ALIAS STM32F4_HAL)
