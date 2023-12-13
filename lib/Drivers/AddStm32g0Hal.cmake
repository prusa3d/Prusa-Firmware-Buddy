add_library(
  STM32G0_HAL
  stm32g0xx_hal_driver/Src/stm32g0xx_hal.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_gpio.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_pwr.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_pwr_ex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_cortex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_tim.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_tim_ex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_uart.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_uart_ex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_rcc.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_rcc_ex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_adc.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_adc_ex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_dma.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_flash.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_flash_ex.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_iwdg.c
  stm32g0xx_hal_driver/Src/stm32g0xx_hal_spi.c
  )

target_include_directories(STM32G0_HAL PUBLIC stm32g0xx_hal_driver/Inc)
target_link_libraries(STM32G0_HAL PUBLIC STM32G0xx::CMSIS STM32G0_HAL_Config)
add_library(STM32G0::HAL ALIAS STM32G0_HAL)
