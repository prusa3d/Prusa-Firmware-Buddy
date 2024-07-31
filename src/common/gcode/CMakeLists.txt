target_include_directories(firmware PRIVATE .)
target_sources(
  firmware
  PUBLIC gcode_buffer.cpp
         gcode_info.cpp
         gcode_info_scan.cpp
         gcode_parser.cpp
         gcode_reader_any.cpp
         gcode_reader_binary.cpp
         gcode_reader_interface.cpp
         gcode_reader_plaintext.cpp
         gcode_thumb_decoder.cpp
         meatpack.cpp
  )
