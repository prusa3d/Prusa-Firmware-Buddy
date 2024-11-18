if(NOT TARGET Marlin_Config)
  message(FATAL_ERROR "Target Marlin_Config does not exist.")
endif()

# Minimal Marlin configuration common to all boards
add_library(
  Marlin
  Marlin/Marlin/src/core/serial.cpp
  Marlin/Marlin/src/core/utility.cpp
  Marlin/Marlin/src/feature/precise_stepping/precise_stepping.cpp
  Marlin/Marlin/src/feature/tmc_util.cpp
  Marlin/Marlin/src/gcode/config/M92.cpp
  Marlin/Marlin/src/gcode/parser.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/HAL.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F4/timers.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F7/timers.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32G0/timers.cpp
  Marlin/Marlin/src/lcd/extensible_ui/ui_api.cpp
  Marlin/Marlin/src/lcd/ultralcd.cpp
  Marlin/Marlin/src/libs/stopwatch.cpp
  Marlin/Marlin/src/Marlin.cpp
  Marlin/Marlin/src/module/configuration_store.cpp
  Marlin/Marlin/src/module/endstops.cpp
  Marlin/Marlin/src/module/motion.cpp
  Marlin/Marlin/src/module/planner.cpp
  Marlin/Marlin/src/module/stepper.cpp
  Marlin/Marlin/src/module/stepper/indirection.cpp
  Marlin/Marlin/src/module/stepper/trinamic.cpp
  Marlin/Marlin/src/module/temperature.cpp
  )

if(BOARD_IS_MASTER_BOARD)
  # Full Marlin configuration for printing (*BUDDY boards)
  target_sources(
    Marlin
    PRIVATE Marlin/Marlin/src/core/multi_language.cpp
            Marlin/Marlin/src/feature/babystep.cpp
            Marlin/Marlin/src/feature/backlash.cpp
            Marlin/Marlin/src/feature/bed_preheat.cpp
            Marlin/Marlin/src/feature/bedlevel/abl/abl.cpp
            Marlin/Marlin/src/feature/bedlevel/bedlevel.cpp
            Marlin/Marlin/src/feature/bedlevel/mbl/mesh_bed_leveling.cpp
            Marlin/Marlin/src/feature/bedlevel/ubl/ubl.cpp
            Marlin/Marlin/src/feature/bedlevel/ubl/ubl_G29.cpp
            Marlin/Marlin/src/feature/bedlevel/ubl/ubl_motion.cpp
            Marlin/Marlin/src/feature/binary_protocol.cpp
            Marlin/Marlin/src/feature/cancel_object.cpp
            Marlin/Marlin/src/feature/host_actions.cpp
            Marlin/Marlin/src/feature/input_shaper/input_shaper.cpp
            Marlin/Marlin/src/feature/input_shaper/input_shaper_config.cpp
            Marlin/Marlin/src/feature/joystick.cpp
            Marlin/Marlin/src/feature/power.cpp
            Marlin/Marlin/src/feature/pressure_advance/pressure_advance.cpp
            Marlin/Marlin/src/feature/pressure_advance/pressure_advance_config.cpp
            Marlin/Marlin/src/feature/print_area.cpp
            Marlin/Marlin/src/feature/prusa/e-stall_detector.cpp
            Marlin/Marlin/src/feature/prusa/measure_axis.cpp
            Marlin/Marlin/src/feature/prusa/restore_z.cpp
            Marlin/Marlin/src/feature/runout.cpp
            Marlin/Marlin/src/feature/spindle_laser.cpp
            Marlin/Marlin/src/feature/touch/xpt2046.cpp
            Marlin/Marlin/src/feature/twibus.cpp
            Marlin/Marlin/src/gcode/bedlevel/abl/G29.cpp
            Marlin/Marlin/src/gcode/bedlevel/abl/M421.cpp
            Marlin/Marlin/src/gcode/bedlevel/G26.cpp
            Marlin/Marlin/src/gcode/bedlevel/G42.cpp
            Marlin/Marlin/src/gcode/bedlevel/M420.cpp
            Marlin/Marlin/src/gcode/bedlevel/mbl/G29.cpp
            Marlin/Marlin/src/gcode/bedlevel/ubl/G29.cpp
            Marlin/Marlin/src/gcode/bedlevel/ubl/M421.cpp
            Marlin/Marlin/src/gcode/calibrate/G28.cpp
            Marlin/Marlin/src/gcode/calibrate/G65.cpp
            Marlin/Marlin/src/gcode/calibrate/G80.cpp
            Marlin/Marlin/src/gcode/calibrate/M666.cpp
            Marlin/Marlin/src/gcode/config/M200-M205.cpp
            Marlin/Marlin/src/gcode/config/M217.cpp
            Marlin/Marlin/src/gcode/config/M218.cpp
            Marlin/Marlin/src/gcode/config/M220.cpp
            Marlin/Marlin/src/gcode/config/M221.cpp
            Marlin/Marlin/src/gcode/config/M301.cpp
            Marlin/Marlin/src/gcode/config/M302.cpp
            Marlin/Marlin/src/gcode/config/M304.cpp
            Marlin/Marlin/src/gcode/config/M305.cpp
            Marlin/Marlin/src/gcode/config/M575.cpp
            Marlin/Marlin/src/gcode/control/M108_M112_M410.cpp
            Marlin/Marlin/src/gcode/control/M111.cpp
            Marlin/Marlin/src/gcode/control/M120_M121.cpp
            Marlin/Marlin/src/gcode/control/M17_M18_M84.cpp
            Marlin/Marlin/src/gcode/control/M211.cpp
            Marlin/Marlin/src/gcode/control/M226.cpp
            Marlin/Marlin/src/gcode/control/M350_M351.cpp
            Marlin/Marlin/src/gcode/control/M42.cpp
            Marlin/Marlin/src/gcode/control/M7-M9.cpp
            Marlin/Marlin/src/gcode/control/M80_M81.cpp
            Marlin/Marlin/src/gcode/control/M85.cpp
            Marlin/Marlin/src/gcode/control/M86.cpp
            Marlin/Marlin/src/gcode/control/M999.cpp
            Marlin/Marlin/src/gcode/eeprom/M500-M504.cpp
            Marlin/Marlin/src/gcode/feature/advance/M900.cpp
            Marlin/Marlin/src/gcode/feature/i2c/M260_M261.cpp
            Marlin/Marlin/src/gcode/feature/input_shaper/M593.cpp
            Marlin/Marlin/src/gcode/feature/input_shaper/M74.cpp
            Marlin/Marlin/src/gcode/feature/modular_bed/M556.cpp
            Marlin/Marlin/src/gcode/feature/modular_bed/M557.cpp
            Marlin/Marlin/src/gcode/feature/pressure_advance/M572.cpp
            Marlin/Marlin/src/gcode/feature/print_area/M555.cpp
            Marlin/Marlin/src/gcode/feature/runout/M412.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M122.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M569.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M906.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M911-M914.cpp
            Marlin/Marlin/src/gcode/gcode.cpp
            Marlin/Marlin/src/gcode/geometry/G53-G59.cpp
            Marlin/Marlin/src/gcode/geometry/G92.cpp
            Marlin/Marlin/src/gcode/geometry/M206_M428.cpp
            Marlin/Marlin/src/gcode/host/M110.cpp
            Marlin/Marlin/src/gcode/host/M113.cpp
            Marlin/Marlin/src/gcode/host/M114.cpp
            Marlin/Marlin/src/gcode/host/M118.cpp
            Marlin/Marlin/src/gcode/host/M119.cpp
            Marlin/Marlin/src/gcode/host/M16.cpp
            Marlin/Marlin/src/gcode/lcd/M0_M1.cpp
            Marlin/Marlin/src/gcode/lcd/M117.cpp
            Marlin/Marlin/src/gcode/lcd/M145.cpp
            Marlin/Marlin/src/gcode/lcd/M300.cpp
            Marlin/Marlin/src/gcode/lcd/M73_PE.cpp
            Marlin/Marlin/src/gcode/motion/G0_G1.cpp
            Marlin/Marlin/src/gcode/motion/G2_G3.cpp
            Marlin/Marlin/src/gcode/motion/G4.cpp
            Marlin/Marlin/src/gcode/motion/G5.cpp
            Marlin/Marlin/src/gcode/motion/M290.cpp
            Marlin/Marlin/src/gcode/motion/M400.cpp
            Marlin/Marlin/src/gcode/probe/G30.cpp
            Marlin/Marlin/src/gcode/probe/M401_M402.cpp
            Marlin/Marlin/src/gcode/probe/M851.cpp
            Marlin/Marlin/src/gcode/queue.cpp
            Marlin/Marlin/src/gcode/sdcard/M20.cpp
            Marlin/Marlin/src/gcode/sdcard/M21_M22.cpp
            Marlin/Marlin/src/gcode/sdcard/M23.cpp
            Marlin/Marlin/src/gcode/sdcard/M24_M25.cpp
            Marlin/Marlin/src/gcode/sdcard/M26.cpp
            Marlin/Marlin/src/gcode/sdcard/M27.cpp
            Marlin/Marlin/src/gcode/sdcard/M28_M29.cpp
            Marlin/Marlin/src/gcode/sdcard/M30.cpp
            Marlin/Marlin/src/gcode/sdcard/M32.cpp
            Marlin/Marlin/src/gcode/sdcard/M524.cpp
            Marlin/Marlin/src/gcode/sdcard/M928.cpp
            Marlin/Marlin/src/gcode/stats/M31.cpp
            Marlin/Marlin/src/gcode/stats/M75-M78.cpp
            Marlin/Marlin/src/gcode/temperature/M104_M109.cpp
            Marlin/Marlin/src/gcode/temperature/M105.cpp
            Marlin/Marlin/src/gcode/temperature/M106_M107.cpp
            Marlin/Marlin/src/gcode/temperature/M140_M190.cpp
            Marlin/Marlin/src/gcode/temperature/M142.cpp
            Marlin/Marlin/src/gcode/temperature/M155.cpp
            Marlin/Marlin/src/gcode/temperature/M303.cpp
            Marlin/Marlin/src/gcode/units/M82_M83.cpp
            Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/eeprom_emul.cpp
            Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/EmulatedEeprom.cpp
            Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/persistent_store_eeprom.cpp
            Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/Servo.cpp
            Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F7/TMC2660.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/backtrace.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwarm.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwarm_arm.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwarm_thumb.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwarmbytab.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwarmmem.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwinder.cpp
            Marlin/Marlin/src/HAL/shared/backtrace/unwmemaccess.cpp
            Marlin/Marlin/src/HAL/shared/eeprom_spi.cpp
            Marlin/Marlin/src/HAL/shared/HAL_spi_L6470.cpp
            Marlin/Marlin/src/HAL/shared/persistent_store_api.cpp
            Marlin/Marlin/src/libs/buzzer.cpp
            Marlin/Marlin/src/libs/crc16.cpp
            Marlin/Marlin/src/libs/hex_print_routines.cpp
            Marlin/Marlin/src/libs/least_squares_fit.cpp
            Marlin/Marlin/src/libs/nozzle.cpp
            Marlin/Marlin/src/libs/numtostr.cpp
            Marlin/Marlin/src/libs/vector_3.cpp
            Marlin/Marlin/src/module/delta.cpp
            Marlin/Marlin/src/module/planner_bezier.cpp
            Marlin/Marlin/src/module/printcounter.cpp
            Marlin/Marlin/src/module/probe.cpp
            Marlin/Marlin/src/module/prusa/homing_utils.cpp
            Marlin/Marlin/src/module/scara.cpp
            Marlin/Marlin/src/module/servo.cpp
            Marlin/Marlin/src/module/stepper/L6470.cpp
            Marlin/Marlin/src/module/stepper/TMC26X.cpp
    )

  if(HAS_REMOTE_ACCELEROMETER OR HAS_LOCAL_ACCELEROMETER)
    target_sources(
      Marlin
      PRIVATE Marlin/Marlin/src/gcode/calibrate/M958.cpp
              Marlin/Marlin/src/module/prusa/accelerometer.cpp
              Marlin/Marlin/src/module/prusa/fourier_series.cpp
      )
  endif()
  if(HAS_LOCAL_ACCELEROMETER)
    target_sources(Marlin PRIVATE Marlin/Marlin/src/module/prusa/accelerometer_local.cpp)
  endif()
  if(HAS_REMOTE_ACCELEROMETER)
    target_sources(
      Marlin PRIVATE Marlin/Marlin/src/module/prusa/accelerometer_remote.cpp
                     Marlin/Marlin/src/module/prusa/accelerometer_utils.cpp
      )
  endif()

  if(HAS_POWER_PANIC OR HAS_CRASH_DETECTION)
    # Power panic/crash detection module
    target_sources(
      Marlin PRIVATE Marlin/Marlin/src/feature/prusa/crash_recovery.cpp
                     Marlin/Marlin/src/feature/prusa/crash_recovery_counters.cpp
      )
  endif()

  if(HAS_PRECISE_HOMING)
    # cartesian precise homing
    target_sources(
      Marlin PRIVATE Marlin/Marlin/src/module/prusa/homing_cart.cpp
                     Marlin/Marlin/src/module/prusa/homing_modus.cpp
      )
  endif()

  if(HAS_PRECISE_HOMING_COREXY)
    # corexy precise homing
    target_sources(Marlin PRIVATE Marlin/Marlin/src/module/prusa/homing_corexy.cpp)
  endif()

  if(PRINTER IN_LIST PRINTERS_WITH_TOOLCHANGER)
    target_sources(
      Marlin
      PRIVATE Marlin/Marlin/src/gcode/control/T.cpp
              Marlin/Marlin/src/module/prusa/spool_join.cpp
              Marlin/Marlin/src/module/prusa/tool_mapper.cpp
              Marlin/Marlin/src/module/prusa/toolchanger.cpp
              Marlin/Marlin/src/module/prusa/toolchanger_utils.cpp
      )
  endif()

  if(PRINTER IN_LIST PRINTERS_WITH_MMU2)
    target_sources(
      Marlin
      PRIVATE Marlin/Marlin/src/feature/prusa/MMU2/mmu2_command_guard.cpp
              Marlin/Marlin/src/feature/prusa/MMU2/mmu2_marlin2.cpp
              Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.cpp
              Marlin/Marlin/src/feature/prusa/MMU2/protocol_logic.cpp
              Marlin/Marlin/src/gcode/control/T.cpp
              Marlin/Marlin/src/gcode/feature/prusa/MMU2/M403.cpp
              Marlin/Marlin/src/module/prusa/spool_join.cpp
              Marlin/Marlin/src/module/prusa/tool_mapper.cpp
              Marlin/Marlin/src/module/tool_change.cpp
      )
  endif()

  if(HAS_PHASE_STEPPING)
    target_sources(
      Marlin
      PRIVATE Marlin/Marlin/src/feature/phase_stepping/calibration.cpp
              Marlin/Marlin/src/feature/phase_stepping/phase_stepping.cpp
              Marlin/Marlin/src/gcode/feature/phase_stepping/M970-M977.cpp
      )
  endif()
endif()

target_compile_features(Marlin PUBLIC cxx_std_17)
target_include_directories(
  Marlin PUBLIC Marlin/Marlin/src Marlin/Marlin/src/gcode/lcd Marlin/Marlin Marlin
  )

target_link_libraries(
  Marlin PUBLIC Arduino::Core Arduino::TMCStepper Marlin_Config error_codes marlin_server_types
                SG14
  )
target_link_libraries(Marlin PRIVATE CppStdExtensions logging freertos)
