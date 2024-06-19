add_executable(freertos_queue_test queue.cpp)

target_include_directories(
  freertos_queue_test PRIVATE . ${CMAKE_SOURCE_DIR}/tests/stubs/freertos
                              ${CMAKE_SOURCE_DIR}/include/common ${CMAKE_SOURCE_DIR}/include
  )

target_link_libraries(freertos_queue_test freertos_tests)

add_catch_test(freertos_queue_test)
