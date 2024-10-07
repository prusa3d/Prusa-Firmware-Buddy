add_subdirectory(startup)

if(BOARD STREQUAL "MODULARBED")
  target_sources(firmware PRIVATE cmsis.cpp sysview.cpp)
else()
  target_sources(firmware PRIVATE cmsis.cpp core_init.cpp hal_msp.cpp peripherals.cpp sysview.cpp)
endif()

if(MCU MATCHES "STM32G070")
  target_set_linker_script(
    firmware "${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32g070rb${BOOT_SUFFIX}.ld"
    )
else()
  message(FATAL_ERROR "Unknown linker script for ${MCU}")
endif()
