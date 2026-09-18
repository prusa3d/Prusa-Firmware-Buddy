# Board-owned HAL and FreeRTOS configuration
add_library(HAL_Config INTERFACE)
target_compile_definitions(HAL_Config INTERFACE STM32G070xx USE_HAL_DRIVER)
target_include_directories(HAL_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
add_library(STM32G0_HAL_Config ALIAS HAL_Config)

target_include_directories(
  BuddyHeaders INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config" "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )

target_sources(
  firmware
  PRIVATE ../../../lib/CrashCatcher/Core/src/CrashCatcher.c
          ../../../lib/CrashCatcher/Core/src/CrashCatcher_armv6m.S
          interrupts.cpp
          main.cpp
          ModbusRegisters.cpp
          otp.cpp
          stm32g0xx_hal_timebase_tim.cpp
  )

target_include_directories(firmware PUBLIC .)

add_subdirectory(control)
add_subdirectory(hal)
