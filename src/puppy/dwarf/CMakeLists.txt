# Board-owned HAL and FreeRTOS configuration. STM32G0_HAL_Config is consumed by STM32G0::HAL; the
# include dirs go on BuddyHeaders because Marlin, Arduino::Core, FreeRTOS and src/* reach device/*.h
# and FreeRTOSConfig.h through it.
add_library(HAL_Config INTERFACE)
target_compile_definitions(HAL_Config INTERFACE STM32G070xx USE_HAL_DRIVER)
target_include_directories(HAL_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
add_library(STM32G0_HAL_Config ALIAS HAL_Config)

target_include_directories(
  BuddyHeaders INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config" "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )

target_sources(
  firmware
  # the two CrashCatcher files need to be here otherwise HardFault_Handler will be handled by
  # Default_Handler (no compiler error)
  PRIVATE ../../../lib/CrashCatcher/Core/src/CrashCatcher.c
          ../../../lib/CrashCatcher/Core/src/CrashCatcher_armv6m.S
          # these two CrashCatcher files need to be here otherwise HardFault_Handler will be handled
          # by Default_Handler (no compiler error)
          accelerometer.cpp
          Cheese.cpp
          interrupts.cpp
          led.cpp
          loadcell.cpp
          logging.cpp
          main.cpp
          module_marlin.cpp
          otp.cpp
          task_startup.cpp
          timing_sys.cpp
          tool_filament_sensor.cpp
  )

target_include_directories(firmware PUBLIC .)

add_subdirectory(modbus)
