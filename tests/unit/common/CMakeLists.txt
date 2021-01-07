add_executable(
  support_utils_tests ${CMAKE_CURRENT_SOURCE_DIR}/support_utils_lib_test.cpp
                      ${CMAKE_SOURCE_DIR}/src/common/support_utils_lib.cpp
  )
target_include_directories(support_utils_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  variant8_tests
  ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp ${CMAKE_SOURCE_DIR}/src/common/variant8.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/malloc_stub.cpp ${CMAKE_CURRENT_SOURCE_DIR}/variant8_tests.cpp
  )
target_include_directories(
  variant8_tests PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/src
  )

add_executable(
  str_utils_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/str_utils_test.cpp ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.hpp
  )
target_include_directories(
  str_utils_tests PUBLIC # ${CMAKE_SOURCE_DIR}/src/common
                         ${CMAKE_SOURCE_DIR}/src
  )

add_executable(
  media_tests ${CMAKE_CURRENT_SOURCE_DIR}/media_test.cpp
              ${CMAKE_SOURCE_DIR}/src/common/gcode_input_filter.hpp
  )
target_include_directories(
  media_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src
  )

add_catch_test(support_utils_tests)
add_catch_test(variant8_tests)
add_catch_test(str_utils_tests)
add_catch_test(media_tests)
