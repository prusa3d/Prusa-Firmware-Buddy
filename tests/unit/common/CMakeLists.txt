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
  fsm_types_test ${CMAKE_CURRENT_SOURCE_DIR}/fsm_types_test.cpp
                 ${CMAKE_SOURCE_DIR}/src/common/fsm_types.cpp
  )
target_include_directories(fsm_types_test PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  fsm_serializers_test ${CMAKE_CURRENT_SOURCE_DIR}/fsm_serializers_test.cpp
                       ${CMAKE_SOURCE_DIR}/src/common/fsm_types.cpp
  )
target_include_directories(
  fsm_serializers_test
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common
  PUBLIC ${CMAKE_SOURCE_DIR}/src/gui/wizard
  )

add_executable(
  gcode_filter_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/gcode_filter_test.cpp ${CMAKE_SOURCE_DIR}/src/common/gcode_filter.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode_filter.hpp
  )
target_include_directories(gcode_filter_tests PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_executable(timing_tests ${CMAKE_SOURCE_DIR}/src/common/timing.c timing_tests.cpp)
target_include_directories(timing_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(algorithm_range_tests algorithm_range_tests.cpp)
target_include_directories(algorithm_range_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(algorithm_scale_tests algorithm_scale_tests.cpp)
target_include_directories(algorithm_scale_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_catch_test(support_utils_tests)
add_catch_test(variant8_tests)
add_catch_test(str_utils_tests)
add_catch_test(fsm_types_test)
add_catch_test(fsm_serializers_test)
add_catch_test(gcode_filter_tests)
add_catch_test(timing_tests)
add_catch_test(algorithm_range_tests)
add_catch_test(algorithm_scale_tests)
