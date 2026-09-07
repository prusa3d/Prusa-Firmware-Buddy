if(NOT TARGET Marlin_Config)
  message(FATAL_ERROR "Target Marlin_Config does not exist.")
endif()

# Minimal Marlin configuration common to all boards
add_library(
  Marlin
  Marlin/Marlin/src/core/serial.cpp
  Marlin/Marlin/src/core/utility.cpp
  Marlin/Marlin/src/feature/motordriver_util.cpp
  Marlin/Marlin/src/feature/tmc_util.cpp
  Marlin/Marlin/src/gcode/parser.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/HAL.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F4/timers.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F7/timers.cpp
  Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32G0/timers.cpp
  Marlin/Marlin/src/lcd/ultralcd.cpp
  Marlin/Marlin/src/libs/stopwatch.cpp
  Marlin/Marlin/src/Marlin.cpp
  Marlin/Marlin/src/module/configuration_store.cpp
  Marlin/Marlin/src/module/stepper/indirection.cpp
  Marlin/Marlin/src/module/stepper/trinamic.cpp
  Marlin/Marlin/src/module/temperature.cpp
  )

add_subdirectory(Marlin/Marlin/src/module/temperature)

if(HAS_PLANNER)
  target_sources(
    Marlin
    PRIVATE Marlin/Marlin/src/feature/precise_stepping/precise_stepping.cpp
            Marlin/Marlin/src/gcode/config/M92.cpp
            Marlin/Marlin/src/module/endstops.cpp
            Marlin/Marlin/src/module/machine_pos_conv.cpp
            Marlin/Marlin/src/module/motion.cpp
            Marlin/Marlin/src/module/planner.cpp
            Marlin/Marlin/src/module/stepper.cpp
    )
endif()

target_compile_options(
  Marlin PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-psabi> $<$<COMPILE_LANGUAGE:CXX>:-Werror=switch>
  )

if(BOARD_IS_MASTER_BOARD)
  # Full Marlin configuration for printing (*BUDDY boards)
  target_sources(
    Marlin
    PRIVATE Marlin/Marlin/src/feature/babystep.cpp
            Marlin/Marlin/src/feature/backlash.cpp
            Marlin/Marlin/src/feature/bedlevel/bedlevel.cpp
            Marlin/Marlin/src/feature/bedlevel/ubl/ubl.cpp
            Marlin/Marlin/src/feature/bedlevel/ubl/ubl_G29.cpp
            Marlin/Marlin/src/feature/host_actions.cpp
            Marlin/Marlin/src/feature/input_shaper/input_shaper.cpp
            Marlin/Marlin/src/feature/input_shaper/input_shaper_config.cpp
            Marlin/Marlin/src/feature/power.cpp
            Marlin/Marlin/src/feature/pressure_advance/pressure_advance.cpp
            Marlin/Marlin/src/feature/pressure_advance/pressure_advance_config.cpp
            Marlin/Marlin/src/feature/print_area.cpp
            Marlin/Marlin/src/feature/prusa/e-stall_detector.cpp
            Marlin/Marlin/src/feature/prusa/measure_axis.cpp
            Marlin/Marlin/src/feature/twibus.cpp
            Marlin/Marlin/src/gcode/bedlevel/G42.cpp
            Marlin/Marlin/src/gcode/bedlevel/M420.cpp
            Marlin/Marlin/src/gcode/bedlevel/ubl/G29.cpp
            Marlin/Marlin/src/gcode/bedlevel/ubl/M421.cpp
            Marlin/Marlin/src/gcode/calibrate/G28.cpp
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
            Marlin/Marlin/src/gcode/config/M575.cpp
            Marlin/Marlin/src/gcode/control/M108_M112_M410.cpp
            Marlin/Marlin/src/gcode/control/M111.cpp
            Marlin/Marlin/src/gcode/control/M120_M121.cpp
            Marlin/Marlin/src/gcode/control/M17_M18_M84.cpp
            Marlin/Marlin/src/gcode/control/M211.cpp
            Marlin/Marlin/src/gcode/control/M350_M351.cpp
            Marlin/Marlin/src/gcode/control/M80_M81.cpp
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
            Marlin/Marlin/src/gcode/feature/trinamic/M122.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M569.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M906.cpp
            Marlin/Marlin/src/gcode/feature/trinamic/M911-M914.cpp
            Marlin/Marlin/src/gcode/gcode.cpp
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
            Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F7/TMC2660.cpp
            Marlin/Marlin/src/libs/hex_print_routines.cpp
            Marlin/Marlin/src/libs/least_squares_fit.cpp
            Marlin/Marlin/src/module/planner_bezier.cpp
            Marlin/Marlin/src/module/printcounter.cpp
            Marlin/Marlin/src/module/probe.cpp
            Marlin/Marlin/src/module/prusa/homing_utils.cpp
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
    target_link_libraries(Marlin PRIVATE LIS2DH12)
  endif()
  if(HAS_REMOTE_ACCELEROMETER)
    target_sources(
      Marlin PRIVATE Marlin/Marlin/src/module/prusa/accelerometer_remote.cpp
                     Marlin/Marlin/src/module/prusa/accelerometer_utils.cpp
      )
    target_link_libraries(Marlin PRIVATE fifo_decoder)
  endif()
  target_link_libraries(Marlin PRIVATE accelerometer)

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

  if(HAS_PRECISE_HOMING_COREXY OR HAS_INDX)
    target_sources(Marlin PRIVATE Marlin/Marlin/src/module/prusa/corexy_transform.cpp)
  endif()

  if(HAS_TOOLCHANGER)
    if(HAS_INDX)
      target_sources(
        Marlin PRIVATE Marlin/Marlin/src/module/prusa/toolchanger_indx.cpp
                       Marlin/Marlin/src/module/prusa/toolchanger_utils_indx.cpp
        )
    else()
      target_sources(
        Marlin PRIVATE Marlin/Marlin/src/module/prusa/toolchanger_utils.cpp
                       Marlin/Marlin/src/module/prusa/toolchanger_xl.cpp
        )
    endif()

    target_sources(
      Marlin
      PRIVATE Marlin/Marlin/src/gcode/control/T.cpp
              Marlin/Marlin/src/module/prusa/spool_join.cpp
              Marlin/Marlin/src/module/prusa/tool_mapper.cpp
              Marlin/Marlin/src/module/prusa/toolchanger_common.cpp
              Marlin/Marlin/src/module/tool_change.cpp
      )
  endif()

  if(HAS_MMU2)
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
      Marlin PRIVATE Marlin/Marlin/src/feature/phase_stepping/phase_stepping.cpp
                     Marlin/Marlin/src/gcode/feature/phase_stepping/M97x.cpp
      )

    if(HAS_PHASE_STEPPING_CALIBRATION)
      target_sources(Marlin PRIVATE Marlin/Marlin/src/feature/phase_stepping/calibration.cpp)
      target_link_libraries(Marlin PRIVATE sfl-library)
    endif()
  endif()

  if(HAS_TOOL_OFFSET_SENSOR)
    target_sources(
      Marlin
      PRIVATE Marlin/Marlin/src/feature/contactless_offset/clo_config.cpp
              Marlin/Marlin/src/feature/contactless_offset/contactless_offset.cpp
              Marlin/Marlin/src/gcode/feature/tool_offset/G426.cpp
      )
    target_link_libraries(Marlin PRIVATE contactless_offset signal_processing sfl-library)
    target_link_libraries(Marlin PRIVATE tool_offset_sensor)
  endif()
endif()

target_compile_features(Marlin PUBLIC cxx_std_17)
target_include_directories(
  Marlin PUBLIC Marlin/Marlin/src Marlin/Marlin/src/gcode/lcd Marlin/Marlin Marlin
  )

target_link_libraries(
  Marlin
  PUBLIC Arduino::Core
         Arduino::TMCStepper
         Marlin_Config
         error_codes
         marlin_server_types
         modbus
         bsod
         SG14
         buddy_utils
         random
         fanctl
         raii
         otp # burst_stepper
         accelerometer
  )
target_link_libraries(Marlin PRIVATE CppStdExtensions logging freertos)

if(HAS_AC_CONTROLLER)
  target_link_libraries(Marlin PRIVATE ac_controller)
endif()

if(HAS_XBUDDY_EXTENSION)
  target_link_libraries(Marlin PUBLIC xbuddy_extension)
endif()

if(RTT_METRICS_ENABLED)
  target_link_libraries(Marlin PUBLIC rtt_metrics_segger)
endif()
