target_include_directories(firmware PRIVATE .)
target_sources(
  firmware PUBLIC gcode_buffer.cpp gcode_filter.cpp gcode_info.cpp gcode_reader.cpp
                  gcode_thumb_decoder.cpp meatpack.cpp
  )
