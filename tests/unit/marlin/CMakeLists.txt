add_executable(
  marlin_temptable_tests
  ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/module/temperature/marlin_temptable.cpp
  marlin_temptable_tests.cpp
  )
target_include_directories(
  marlin_temptable_tests PRIVATE stub_include_temptable ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src
  )
add_catch_test(marlin_temptable_tests)

add_executable(
  machine_pos_conv_tests ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/module/machine_pos_conv.cpp
                         machine_pos_conv_tests.cpp
  )
target_include_directories(
  machine_pos_conv_tests PRIVATE stub_include_pos_conv ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src
  )
target_link_libraries(machine_pos_conv_tests buddy_utils)
add_catch_test(machine_pos_conv_tests)

add_executable(vector_tests vector_tests.cpp)
target_include_directories(vector_tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src)
target_link_libraries(vector_tests buddy_utils)
add_catch_test(vector_tests)
