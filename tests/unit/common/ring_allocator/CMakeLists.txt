add_executable(
  ring_allocator_tests ${CMAKE_SOURCE_DIR}/src/common/ring_allocator.cpp ring_allocator_test.cpp
  )

target_include_directories(
  ring_allocator_tests PRIVATE . ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/tests/stubs
  )
add_catch_test(ring_allocator_tests)
