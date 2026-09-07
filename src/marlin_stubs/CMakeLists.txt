target_sources(
  firmware
  PRIVATE G123.cpp
          G26.cpp
          gcode.cpp
          M104_1.cpp
          M123.cpp
          M1702.cpp
          M300.cpp
          M330.cpp
          M340.cpp
          M591.cpp
          M862_1.cpp
          M862_4.cpp
          M863.cpp
          M865.cpp
          M876.cpp
          M9140_M9150.cpp
          M9200.cpp
          M9201.cpp
          M9933.cpp
          M997.cpp
          M999.cpp
          skippable_gcode.cpp
  )

if(NOT BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(firmware PRIVATE G26.cpp M862_2_3.cpp M862_5.cpp M862_6.cpp)
endif()

if(HAS_TRINAMIC)
  target_sources(firmware PRIVATE M919-M920.cpp)
endif()

if(HAS_CANCEL_OBJECT)
  target_sources(firmware PRIVATE M486.cpp)
endif()

if(HAS_I2C_EXPANDER)
  target_sources(firmware PRIVATE M262-M268.cpp)
endif()

add_subdirectory(pause)
add_subdirectory(sdcard)
add_subdirectory(host)
add_subdirectory(feature)
add_subdirectory(${PRINTER})

if(HAS_NOZZLE_CLEANER OR HAS_NOZZLE_CLEANER_LITE)
  target_sources(firmware PRIVATE G12.cpp)
endif()

if(HAS_SELFTEST)
  target_sources(firmware PRIVATE G162.cpp G163.cpp M1978.cpp M1981.cpp)
endif()

if(HAS_HEATERS_SELFTEST_GCODE)
  target_sources(firmware PRIVATE M1987.cpp)
endif()

if(HAS_LEDS)
  target_sources(firmware PRIVATE M150.cpp)
endif()

if(HAS_SPOOL_JOIN)
  target_sources(firmware PRIVATE M864.cpp)
endif()

if(HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE P.cpp)
endif()

if(HAS_TOOL_OFFSET_PIN_CALIBRATION)
  target_sources(firmware PRIVATE G425.cpp)
endif()

if(HAS_GUI)
  target_sources(firmware PRIVATE M0.cpp)
endif()

if(HAS_PHASE_STEPPING_CALIBRATION)
  target_sources(firmware PRIVATE M1977.cpp)
endif()

if(HAS_INPUT_SHAPER_CALIBRATION)
  target_sources(firmware PRIVATE M1959.cpp)
endif()

if(HAS_MANUAL_BELT_TUNING)
  target_sources(firmware PRIVATE M961.cpp)
endif()

if(CONNECT)
  target_sources(firmware PRIVATE M1200.cpp)
endif()

if(HAS_CHAMBER_FILTRATION_API)
  target_sources(firmware PRIVATE M147_148.cpp)
endif()

if(HAS_DOOR_SENSOR_CALIBRATION)
  target_sources(firmware PRIVATE M1980.cpp)
endif()

if(HAS_INDX)
  target_sources(firmware PRIVATE M1982.cpp M1983.cpp M1984.cpp)
endif()

if(HAS_WASTEBIN_FILL_TRACKING)
  target_sources(firmware PRIVATE M1986.cpp)
endif()

if(HAS_TOOL_OFFSET_SENSOR)
  target_sources(firmware PRIVATE G427.cpp M1985.cpp)
endif()

if(HAS_PRECISE_HOMING_COREXY)
  target_sources(firmware PRIVATE M9202.cpp)
endif()

if(HAS_ESP)
  target_sources(firmware PRIVATE M1703.cpp)
endif()

if(HAS_AUTO_RETRACT)
  target_sources(firmware PRIVATE M1705.cpp)
endif()

if(HAS_CHAMBER_VENTS)
  target_sources(firmware PRIVATE M870.cpp)
endif()

if(HAS_INDX)
  target_sources(firmware PRIVATE G750.cpp)
endif()

if(HAS_TOOLCHANGER OR HAS_MMU2)
  target_sources(firmware PRIVATE M9934.cpp)
endif()

if(HAS_CRASH_DETECTION)
  target_sources(firmware PRIVATE M9944.cpp)
endif()
