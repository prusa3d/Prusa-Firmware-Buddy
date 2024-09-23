add_executable(
  test_parser ${CMAKE_SOURCE_DIR}/lib/WUI/esp_protocol/parser.cpp
              ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp test_parser.cpp
  )

target_include_directories(
  test_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lib/WUI/
                      ${CMAKE_SOURCE_DIR}/src
  )

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/uart.dump DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_catch_test(test_parser)
