target_sources(
  firmware
  PRIVATE ../../../lib/CrashCatcher/Core/src/CrashCatcher.c
          ../../../lib/CrashCatcher/Core/src/CrashCatcher_armv6m.S
          interrupts.cpp
          main.cpp
          ModbusRegisters.cpp
          stm32g0xx_hal_timebase_tim.cpp
          syscalls.cpp
  )

target_include_directories(firmware PUBLIC .)

add_subdirectory(control)
add_subdirectory(hal)
