add_subdirectory(probe_analysis)
add_subdirectory(automata)
add_subdirectory(Leds)
add_subdirectory(gcode)
add_subdirectory(e2ee)
add_subdirectory(http)
add_subdirectory(utils)
add_subdirectory(ring_allocator)
add_subdirectory(filament_sensor)
add_subdirectory(freertos)
add_subdirectory(st25dv64k)

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
  str_utils_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src
                         ${CMAKE_SOURCE_DIR}/src/lang
  )

add_executable(
  string_builder_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/string_builder_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/common/utils/string_builder.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  )
target_include_directories(
  string_builder_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src
                              ${CMAKE_SOURCE_DIR}/src/lang
  )
add_catch_test(string_builder_tests)

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

add_executable(algorithm_scale_tests algorithm_scale_tests.cpp)
target_include_directories(algorithm_scale_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(median_tests ${CMAKE_CURRENT_SOURCE_DIR}/median_test.cpp)
target_include_directories(median_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(median_tests buddy_utils)

add_executable(
  otp_tests ${CMAKE_CURRENT_SOURCE_DIR}/otp_test.cpp ${CMAKE_SOURCE_DIR}/src/common/otp.cpp
  )
target_compile_definitions(otp_tests PUBLIC UNITTEST)
target_include_directories(otp_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(otp_tests otp)

add_executable(
  url_decode_tests
  ${CMAKE_SOURCE_DIR}/src/common/http/url_decode.cpp ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/url_decode_tests.cpp
  )
target_include_directories(
  url_decode_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/lang
  )

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
  ${CMAKE_SOURCE_DIR}/src/common/path_utils.cpp ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/path_utils.cpp
  )
target_include_directories(
  path_utils_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/include
  )

add_executable(exponential_backoff_tests ${CMAKE_CURRENT_SOURCE_DIR}/exponential_backoff_tests.cpp)
target_include_directories(exponential_backoff_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)

add_executable(hotend_detect_tests ${CMAKE_CURRENT_SOURCE_DIR}/hotend_detect_tests.cpp)
target_include_directories(hotend_detect_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)
add_catch_test(hotend_detect_tests)

add_catch_test(support_utils_tests)
add_catch_test(str_utils_tests)
add_catch_test(timing_tests)
add_catch_test(circle_buffer_tests)
add_catch_test(circular_buffer_tests)
add_catch_test(weighted_modus_tests)
add_catch_test(algorithm_scale_tests)
add_catch_test(median_tests)
add_catch_test(otp_tests)
add_catch_test(url_decode_tests)
add_catch_test(json_tests)
add_catch_test(path_utils_tests)
add_catch_test(exponential_backoff_tests)

add_executable(encoded_bitset_tests ${CMAKE_CURRENT_SOURCE_DIR}/encoded_bitset_test.cpp)
target_include_directories(
  encoded_bitset_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(encoded_bitset_tests)

add_executable(
  encoded_filament_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/encoded_filament_test.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  ${CMAKE_SOURCE_DIR}/src/persistent_stores/store_instances/config_store/filament_variant_decode.cpp
  )
target_include_directories(
  encoded_filament_tests
  PUBLIC # Stubs must come first to shadow headers with heavy dependencies (e.g. tool_index.hpp →
         # Marlin)
         ${CMAKE_CURRENT_SOURCE_DIR}/encoded_filament_stubs
         ${CMAKE_SOURCE_DIR}/src/persistent_stores/store_instances ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/lang ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(encoded_filament_tests)
