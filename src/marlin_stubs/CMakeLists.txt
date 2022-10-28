target_sources(
  firmware
  PRIVATE G163.cpp
          G26.cpp
          gcode.cpp
          M300.cpp
          M330.cpp
          M505.cpp
          M876.cpp
          M997.cpp
          M999.cpp
  )

add_subdirectory(pause)
add_subdirectory(sdcard)
add_subdirectory(host)
add_subdirectory(feature)

if(WUI)
  target_sources(firmware PRIVATE M1587.cpp)
endif()
