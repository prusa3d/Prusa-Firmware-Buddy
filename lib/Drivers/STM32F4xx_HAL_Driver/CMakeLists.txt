if(NOT STM32F4_HAL_TARGET)
  message(FATAL_ERROR "Please set STM32F4_HAL_TARGET")
endif()
if(NOT TARGET STM32F4_HAL_Config)
  message(FATAL_ERROR "Target STM32F4_HAL_Config does not exist.")
endif()

add_library(
  HAL
  Src/stm32f4xx_hal.c
  Src/stm32f4xx_hal_adc.c
  Src/stm32f4xx_hal_adc_ex.c
  Src/stm32f4xx_hal_cortex.c
  Src/stm32f4xx_hal_dma.c
  Src/stm32f4xx_hal_dma_ex.c
  Src/stm32f4xx_hal_eth.c
  Src/stm32f4xx_hal_flash.c
  Src/stm32f4xx_hal_flash_ex.c
  Src/stm32f4xx_hal_flash_ramfunc.c
  Src/stm32f4xx_hal_gpio.c
  Src/stm32f4xx_hal_hcd.c
  Src/stm32f4xx_hal_i2c.c
  Src/stm32f4xx_hal_i2c_ex.c
  Src/stm32f4xx_hal_iwdg.c
  Src/stm32f4xx_hal_pcd.c
  Src/stm32f4xx_hal_pcd_ex.c
  Src/stm32f4xx_hal_pwr.c
  Src/stm32f4xx_hal_pwr_ex.c
  Src/stm32f4xx_hal_rcc.c
  Src/stm32f4xx_hal_rcc_ex.c
  Src/stm32f4xx_hal_rng.c
  Src/stm32f4xx_hal_rtc.c
  Src/stm32f4xx_hal_rtc_ex.c
  Src/stm32f4xx_hal_spi.c
  Src/stm32f4xx_hal_tim.c
  Src/stm32f4xx_hal_tim_ex.c
  Src/stm32f4xx_hal_uart.c
  Src/stm32f4xx_hal_wwdg.c
  Src/stm32f4xx_ll_usb.c
  )

target_include_directories(HAL PUBLIC Inc)

target_compile_definitions(HAL PUBLIC ${STM32F4_HAL_TARGET} USE_HAL_DRIVER)

target_link_libraries(HAL PUBLIC CMSIS STM32F4_HAL_Config)
add_library(STM32F4::HAL ALIAS HAL)
