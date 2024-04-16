add_executable(freertos_queue_test queue.cpp)

target_include_directories(
  freertos_queue_test PRIVATE . ${CMAKE_SOURCE_DIR}/tests/stubs/freertos
                              ${CMAKE_SOURCE_DIR}/include/common
  )

target_link_libraries(freertos_queue_test freertos)

add_catch_test(freertos_queue_test)
