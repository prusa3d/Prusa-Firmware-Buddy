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
          fifo_encoder.cpp
          interrupts.cpp
          led.cpp
          loadcell.cpp
          logging.cpp
          main.cpp
          module_marlin.cpp
          task_startup.cpp
          timing_sys.cpp
          tool_filament_sensor.cpp
  )

target_include_directories(firmware PUBLIC .)

add_subdirectory(modbus)
