add_library(STM32H5_HAL_Config INTERFACE)
target_compile_definitions(STM32H5_HAL_Config INTERFACE STM32H503xx USE_HAL_DRIVER)
target_include_directories(STM32H5_HAL_Config INTERFACE config)

target_compile_definitions(firmware PRIVATE STM32GENERIC STM32H5 STM32H5xx)

target_link_options(firmware PUBLIC "-L" "${CMAKE_CURRENT_SOURCE_DIR}")
if(BOOTLOADER STREQUAL "EMPTY" OR BOOTLOADER)
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/stm32h503_boot.ld")
  target_compile_definitions(firmware PRIVATE BOOTLOADER_SIZE=8192)
else()
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/stm32h503_noboot.ld")
  target_compile_definitions(firmware PRIVATE BOOTLOADER_SIZE=0)
endif()

target_sources(
  firmware
  PRIVATE app.cpp
          cmsis.cpp
          hal.cpp
          main.cpp
          modbus.cpp
          stdlib.c
          stm32h503.s
          temperature.cpp
  )
target_link_libraries(firmware PRIVATE printf::printf STM32H5::HAL STM32H5_HAL_Config freertos)

# TODO For now, we need to also explicitly depend on `FreeRTOS::FreeRTOS` because we are using
# functions that are not wrapped by `freertos`.
target_link_libraries(firmware PRIVATE FreeRTOS::FreeRTOS)

add_library(FreeRTOS_Config INTERFACE)
target_include_directories(FreeRTOS_Config INTERFACE config)
