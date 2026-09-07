target_include_directories(firmware PRIVATE .)
target_sources(
  firmware
  PUBLIC gcode_basic_parser.cpp
         gcode_buffer.cpp
         gcode_info.cpp
         gcode_info_check.cpp
         gcode_info_scan.cpp
         gcode_loader.cpp
         gcode_parser.cpp
         gcode_parser_helper.cpp
         gcode_reader_any.cpp
         gcode_reader_binary.cpp
         gcode_reader_interface.cpp
         gcode_reader_plaintext.cpp
         inject_queue.cpp
         inject_queue_actions.cpp
         meatpack.cpp
  )
