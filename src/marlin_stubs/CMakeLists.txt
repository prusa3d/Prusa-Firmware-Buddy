target_sources(
  firmware
  PRIVATE G26.cpp
          G64.cpp
          gcode.cpp
          M123.cpp
          M300.cpp
          M330.cpp
          M340.cpp
          M486.cpp
          M591.cpp
          M862_1.cpp
          M862_2.cpp
          M862_3.cpp
          M862_4.cpp
          M862_5.cpp
          M862_6.cpp
          M863.cpp
          M864.cpp
          M876.cpp
          M919-M920.cpp
          M93X.cpp
          M997.cpp
          M999.cpp
  )

add_subdirectory(pause)
add_subdirectory(sdcard)
add_subdirectory(host)
add_subdirectory(feature)
add_subdirectory(${PRINTER})

if(HAS_SELFTEST)
  target_sources(firmware PRIVATE G162.cpp G163.cpp)
  if(WUI)
    target_sources(firmware PRIVATE M1587.cpp)
  endif()
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
