add_executable(
  fifo_coder_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/fifo_coder_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/puppy/dwarf/fifo_encoder.cpp
  ${CMAKE_SOURCE_DIR}/src/puppies/fifo_decoder.cpp
  )

target_include_directories(
  fifo_coder_tests PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/tests/stubs
  )

add_catch_test(fifo_coder_tests)

add_executable(
  time_sync ${CMAKE_CURRENT_SOURCE_DIR}/time_sync.cpp ${CMAKE_SOURCE_DIR}/src/puppies/time_sync.cpp
            ${CMAKE_SOURCE_DIR}/src/common/filters/kalman.cpp
  )

target_include_directories(
  time_sync PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/include
                   ${CMAKE_SOURCE_DIR}/src/common
  )

add_catch_test(time_sync)
