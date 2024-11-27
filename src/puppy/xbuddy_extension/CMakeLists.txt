add_library(STM32H5_HAL_Config INTERFACE)
target_compile_definitions(STM32H5_HAL_Config INTERFACE STM32H503xx USE_HAL_DRIVER)
target_include_directories(STM32H5_HAL_Config INTERFACE config)

# TODO It would be better if Prusa-Firmware-MMU itself declared this library, but ok.
set(PRUSA_FIRMWARE_MMU_PATH ${CMAKE_SOURCE_DIR}/lib/Prusa-Firmware-MMU)
add_library(mmu_protocol OBJECT ${PRUSA_FIRMWARE_MMU_PATH}/src/modules/protocol.cpp)
target_include_directories(mmu_protocol PUBLIC ${PRUSA_FIRMWARE_MMU_PATH}/src)
target_compile_options(mmu_protocol PRIVATE -Wno-implicit-fallthrough)

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
          hal_clock.cpp
          main.cpp
          mmu.cpp
          modbus.cpp
          stdlib.c
          stm32h503.s
          temperature.cpp
  )
target_link_libraries(
  firmware PRIVATE printf::printf STM32H5::HAL STM32H5_HAL_Config freertos XBuddyExtensionShared
                   mmu_protocol
  )

# TODO For now, we need to also explicitly depend on `FreeRTOS::FreeRTOS` because we are using
# functions that are not wrapped by `freertos`.
target_link_libraries(firmware PRIVATE FreeRTOS::FreeRTOS)

add_library(FreeRTOS_Config INTERFACE)
target_include_directories(FreeRTOS_Config INTERFACE config)
