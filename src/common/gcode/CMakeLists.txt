target_include_directories(firmware PRIVATE .)
target_sources(
  firmware PUBLIC gcode_file.cpp gcode_filter.cpp gcode_info.cpp gcode_thumb_decoder.cpp
  )
