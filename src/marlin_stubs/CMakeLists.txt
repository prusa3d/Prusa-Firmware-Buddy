target_sources(
  firmware
  PRIVATE gcode.cpp
          G26.cpp
          G163.cpp
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
