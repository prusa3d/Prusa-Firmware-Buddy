add_subdirectory(startup)

target_sources(
  firmware PRIVATE core_init.cpp hal_msp.cpp interrupts_common.cpp interrupts_helper.cpp
                   peripherals.cpp peripherals_uart.cpp
  )

if(BOARD STREQUAL "BUDDY")
  target_sources(firmware PRIVATE interrupts_BUDDY.cpp)
elseif(BOARD STREQUAL "XBUDDY")
  target_sources(firmware PRIVATE interrupts_XBUDDY.cpp)
elseif(BOARD STREQUAL "XLBUDDY")
  target_sources(firmware PRIVATE interrupts_XLBUDDY.cpp)
else()
  message(FATAL_ERROR "Unknown board")
endif()

if(BOOTLOADER)
  target_sources(firmware PRIVATE cmsis_boot.cpp)
else()
  target_sources(firmware PRIVATE cmsis.cpp)
endif()

if(MCU MATCHES "STM32F42")
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f42x${BOOT_SUFFIX}.ld")
elseif(MCU MATCHES "STM32F407")
  target_set_linker_script(
    firmware "${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f407vg${BOOT_SUFFIX}.ld"
    )
else()
  message(FATAL_ERROR "Unknown linker script for ${MCU}")
endif()
