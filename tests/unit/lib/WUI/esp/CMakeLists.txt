add_catch_test(test_checksum)

add_executable(
  test_parser ${CMAKE_SOURCE_DIR}/lib/WUI/esp_protocol/parser.cpp
              ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp test_parser.cpp
  )
target_link_libraries(test_parser INTERFACE Catch2)
# target_link_libraries(test_parser asan)

# target_compile_options(test_parser PUBLIC -fsanitize=address,leak)

target_include_directories(
  test_parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/lib/WUI/
                        ${CMAKE_SOURCE_DIR}/src
  )

add_catch_test(test_parser)
