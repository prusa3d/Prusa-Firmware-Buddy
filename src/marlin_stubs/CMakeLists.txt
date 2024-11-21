target_sources(
  firmware
  PRIVATE G123.cpp
          G26.cpp
          G64.cpp
          gcode.cpp
          M104_1.cpp
          M123.cpp
          M1702.cpp
          M1703.cpp
          M300.cpp
          M330.cpp
          M340.cpp
          M486.cpp
          M591.cpp
          M862_1.cpp
          M862_4.cpp
          M863.cpp
          M864.cpp
          M865.cpp
          M876.cpp
          M9140_M9150.cpp
          M919-M920.cpp
          M9200.cpp
          M9201.cpp
          M997.cpp
          M999.cpp
  )

if(NOT BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(firmware PRIVATE G26.cpp M862_2_3.cpp M862_5.cpp M862_6.cpp)
endif()

if(HAS_I2C_EXPANDER)
  target_sources(firmware PRIVATE M262-M268.cpp)
endif()

add_subdirectory(pause)
add_subdirectory(sdcard)
add_subdirectory(host)
add_subdirectory(feature)
add_subdirectory(${PRINTER})

if(HAS_NOZZLE_CLEANER)
  target_sources(firmware PRIVATE G12.cpp)
endif()

if(HAS_SELFTEST)
  target_sources(firmware PRIVATE G162.cpp G163.cpp)
endif()

if(HAS_LEDS)
  target_sources(firmware PRIVATE M150.cpp)
endif()

if(HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE G425.cpp P.cpp)
endif()

if(HAS_GUI)
  target_sources(firmware PRIVATE M0.cpp)
endif()

if(HAS_PHASE_STEPPING)
  target_sources(firmware PRIVATE M1977.cpp)
endif()

if(HAS_INPUT_SHAPER_CALIBRATION)
  target_sources(firmware PRIVATE M1959.cpp)
endif()

if(HAS_BELT_TUNING)
  target_sources(firmware PRIVATE M960.cpp)
endif()

if(CONNECT)
  target_sources(firmware PRIVATE M1200.cpp)
endif()
