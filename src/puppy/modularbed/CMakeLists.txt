target_sources(
  firmware
  PRIVATE ../../../lib/CrashCatcher/Core/src/CrashCatcher.c
          ../../../lib/CrashCatcher/Core/src/CrashCatcher_armv6m.S
          crash_catcher_integration.cpp
          interrupts.cpp
          main.cpp
          ModbusRegisters.cpp
          stm32g0xx_hal_timebase_tim.cpp
          syscalls.c
          trigger_crash_dump.c
  )

target_include_directories(firmware PUBLIC .)

add_subdirectory(control)
add_subdirectory(hal)
