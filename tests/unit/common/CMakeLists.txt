add_subdirectory(automata)
add_subdirectory(http)

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
target_include_directories(
  fsm_types_test
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common
  )

add_executable(
  fsm_serializers_test ${CMAKE_CURRENT_SOURCE_DIR}/fsm_serializers_test.cpp
                       ${CMAKE_SOURCE_DIR}/src/common/fsm_types.cpp
  )
target_include_directories(
  fsm_serializers_test
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
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
target_compile_definitions(timing_tests PRIVATE MCU=STM32F407VG)
target_include_directories(
  timing_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/include
                      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

add_executable(
  weighted_modus_tests ${CMAKE_CURRENT_SOURCE_DIR}/weighted_modus_test.cpp
                       ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/module/precise_homing.cpp
  )
target_include_directories(
  weighted_modus_tests PUBLIC ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/module/
  )

add_executable(algorithm_range_tests algorithm_range_tests.cpp)
target_include_directories(algorithm_range_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(algorithm_scale_tests algorithm_scale_tests.cpp)
target_include_directories(algorithm_scale_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(median_tests ${CMAKE_CURRENT_SOURCE_DIR}/median_test.cpp)
target_include_directories(median_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  url_decode_tests ${CMAKE_SOURCE_DIR}/src/common/http/url_decode.cpp
                   ${CMAKE_CURRENT_SOURCE_DIR}/url_decode_tests.cpp
  )
target_include_directories(url_decode_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  json_tests
  ${CMAKE_SOURCE_DIR}/src/common/jsmn.c
  ${CMAKE_CURRENT_SOURCE_DIR}/search_json_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/json_encode_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/segmented_json_rendering.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.c
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  )
target_include_directories(
  json_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/lib/jsmn
  )

add_catch_test(support_utils_tests)
add_catch_test(variant8_tests)
add_catch_test(str_utils_tests)
add_catch_test(fsm_types_test)
add_catch_test(fsm_serializers_test)
add_catch_test(gcode_filter_tests)
add_catch_test(timing_tests)
add_catch_test(weighted_modus_tests)
add_catch_test(algorithm_range_tests)
add_catch_test(algorithm_scale_tests)
add_catch_test(median_tests)
add_catch_test(url_decode_tests)
add_catch_test(json_tests)
