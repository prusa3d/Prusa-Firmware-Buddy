add_subdirectory(probe_analysis)
add_subdirectory(automata)
add_subdirectory(Leds)
add_subdirectory(gcode)
add_subdirectory(http)
add_subdirectory(utils)
add_subdirectory(filament_sensor)
add_subdirectory(freertos)

add_executable(
  support_utils_tests ${CMAKE_CURRENT_SOURCE_DIR}/support_utils_lib_test.cpp
                      ${CMAKE_SOURCE_DIR}/src/common/support_utils_lib.cpp
  )
target_include_directories(support_utils_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  str_utils_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/str_utils_test.cpp ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.hpp ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  )
target_include_directories(
  str_utils_tests PUBLIC # ${CMAKE_SOURCE_DIR}/src/common
                         ${CMAKE_SOURCE_DIR}/src
  )

add_executable(timing_tests timing_tests.cpp)
target_compile_definitions(timing_tests PRIVATE MCU=STM32F407VG)
target_include_directories(
  timing_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/include
                      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

add_executable(circle_buffer_tests circle_buffer_test.cpp)
target_include_directories(
  circle_buffer_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/include
  )

add_executable(circular_buffer_tests circular_buffer_test.cpp)
target_include_directories(circular_buffer_tests PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(
  weighted_modus_tests ${CMAKE_CURRENT_SOURCE_DIR}/weighted_modus_test.cpp
                       ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/module/prusa/homing_modus.cpp
  )
target_include_directories(
  weighted_modus_tests PUBLIC ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/module/prusa/
  )

add_executable(
  algorithm_range_tests ${CMAKE_SOURCE_DIR}/src/common/filters/median_filter.cpp
                        algorithm_range_tests.cpp
  )
target_include_directories(algorithm_range_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(algorithm_scale_tests algorithm_scale_tests.cpp)
target_include_directories(algorithm_scale_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  median_tests ${CMAKE_CURRENT_SOURCE_DIR}/median_test.cpp
               ${CMAKE_SOURCE_DIR}/src/common/filters/median_filter.cpp
  )
target_include_directories(median_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  otp_tests ${CMAKE_CURRENT_SOURCE_DIR}/otp_test.cpp ${CMAKE_SOURCE_DIR}/src/common/otp.cpp
  )
target_compile_definitions(otp_tests PUBLIC UNITTEST)
target_include_directories(otp_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  url_decode_tests ${CMAKE_SOURCE_DIR}/src/common/http/url_decode.cpp
                   ${CMAKE_CURRENT_SOURCE_DIR}/url_decode_tests.cpp
  )
target_include_directories(url_decode_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  json_tests
  ${CMAKE_SOURCE_DIR}/src/common/jsmn.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/search_json_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/json_encode_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/segmented_json_rendering.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.cpp
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  )
target_include_directories(
  json_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/lib/jsmn
  )

add_executable(
  path_utils_tests
  ${CMAKE_SOURCE_DIR}/src/common/path_utils.cpp ${CMAKE_SOURCE_DIR}/src/common/stat_retry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/path_utils.cpp
  )
target_include_directories(path_utils_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(
  selftest_test_data_serialization_tests
  ${CMAKE_SOURCE_DIR}/src/common/selftest/selftest_data.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/selftest_data_serialization.cpp
  )

target_include_directories(
  selftest_test_data_serialization_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common/selftest/include_XL
         ${CMAKE_SOURCE_DIR}/src/common/marlin_server_types ${CMAKE_SOURCE_DIR}/tests/stubs
  )

add_catch_test(support_utils_tests)
add_catch_test(str_utils_tests)
add_catch_test(timing_tests)
add_catch_test(circle_buffer_tests)
add_catch_test(circular_buffer_tests)
add_catch_test(weighted_modus_tests)
add_catch_test(algorithm_range_tests)
add_catch_test(algorithm_scale_tests)
add_catch_test(median_tests)
add_catch_test(otp_tests)
add_catch_test(url_decode_tests)
add_catch_test(json_tests)
add_catch_test(path_utils_tests)
add_catch_test(selftest_test_data_serialization_tests)
