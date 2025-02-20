add_executable(
  ramming_sequence_tests ${CMAKE_SOURCE_DIR}/src/common/feature/ramming/ramming_sequence.cpp
                         ramming_sequence_tests.cpp
  )
target_include_directories(
  ramming_sequence_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_CURRENT_SOURCE_DIR}/stub
                                 ${CMAKE_SOURCE_DIR}/lib/SG14
  )
add_catch_test(ramming_sequence_tests)
