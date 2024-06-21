add_executable(
  media_prefetch_tests
  ${CMAKE_SOURCE_DIR}/src/common/media_prefetch/media_prefetch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/media_prefetch_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stub/gcode/gcode_reader_any.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stub/async_job/async_job.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stub/test_tools/gcode_provider.cpp
  )

target_include_directories(
  media_prefetch_tests PUBLIC . ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/stub
                              ${CMAKE_SOURCE_DIR}/src/common/media_prefetch
  )

add_catch_test(media_prefetch_tests)
