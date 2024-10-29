add_executable(
  gcode_basic_parser_tests
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_basic_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_parser_helper.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gcode_basic_parser_tests.cpp
  )
target_include_directories(
  gcode_basic_parser_tests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stub ${CMAKE_SOURCE_DIR}/src/common/gcode
         ${CMAKE_SOURCE_DIR}/lib/SG14 ${CMAKE_SOURCE_DIR}/src/common/ ${CMAKE_SOURCE_DIR}/src/lang
  )
add_catch_test(gcode_basic_parser_tests)

add_executable(
  gcode_parser_tests
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_basic_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_parser_helper.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gcode_parser_tests.cpp
  )
target_include_directories(
  gcode_parser_tests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stub ${CMAKE_SOURCE_DIR}/src/common/gcode
         ${CMAKE_SOURCE_DIR}/lib/SG14 ${CMAKE_SOURCE_DIR}/src/common/ ${CMAKE_SOURCE_DIR}/src/lang
  )
add_catch_test(gcode_parser_tests)

add_executable(
  gcode_parser_custom_types_tests
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_basic_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_parser_helper.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filament.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gcode_parser_custom_types_tests.cpp
  )
target_include_directories(
  gcode_parser_custom_types_tests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stub
         ${CMAKE_SOURCE_DIR}/src/common/gcode
         ${CMAKE_SOURCE_DIR}/lib/SG14
         ${CMAKE_SOURCE_DIR}/src/common/
         ${CMAKE_SOURCE_DIR}/src/lang
         ${CMAKE_SOURCE_DIR}/include
         ${OPTIONS_INCLUDE_DIR}
  )
add_catch_test(gcode_parser_custom_types_tests)
