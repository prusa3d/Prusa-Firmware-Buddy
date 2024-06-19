add_executable(
  gcode_reader_test
  ${CMAKE_SOURCE_DIR}/src/common/base64_stream_decoder.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_buffer.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_any.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_binary.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_interface.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_plaintext.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/meatpack.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/lib/heatshrink/heatshrink_decoder.c
  gcode_reader.cpp
  missing_functions.c
  )
target_include_directories(
  gcode_reader_test
  PRIVATE .
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/common/gcode
          ${CMAKE_SOURCE_DIR}/lib
          ${CMAKE_SOURCE_DIR}/lib/heatshrink
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/include/usb_host
          ${CMAKE_SOURCE_DIR}/lib/SG14
  )

target_link_libraries(gcode_reader_test mbedTLS bgcode_core freertos_tests)

file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_plain.gcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_no_compression.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_meatpack.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_heatshrink.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_heatshrink_meatpack.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_bad_crc_first_gcode.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_bad_crc_intro.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_bad_crc_gcode.bgcode
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  )

add_catch_test(gcode_reader_test)
