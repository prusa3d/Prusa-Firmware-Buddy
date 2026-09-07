add_executable(
  media_prefetch_tests
  ${CMAKE_SOURCE_DIR}/src/common/media_prefetch/media_prefetch.cpp
  ${CMAKE_SOURCE_DIR}/src/common/media_prefetch/prefetch_compression.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/async_job/async_job.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/media_prefetch_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stub/gcode/gcode_reader_any.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stub/test_tools/gcode_provider.cpp
  )

target_include_directories(
  media_prefetch_tests
  PUBLIC .
         ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/stub
         ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/src/common/media_prefetch
         ${CMAKE_BINARY_DIR}/include
         ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_SOURCE_DIR}/src/common
  )

file(
  GLOB_RECURSE DATA_FILES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  data/*
  )
foreach(filename ${DATA_FILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_CURRENT_BINARY_DIR}/${filename} COPYONLY
    )
endforeach(filename)

target_link_libraries(media_prefetch_tests buddy_utils freertos-mock logging-mock)

add_catch_test(media_prefetch_tests)
