add_executable(loadcell_deglitcher_tests tests.cpp)

target_link_libraries(loadcell_deglitcher_tests fifo_coder_base)

target_include_directories(
  loadcell_deglitcher_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/puppy/dwarf ${CMAKE_SOURCE_DIR}/include
  )

add_catch_test(loadcell_deglitcher_tests)
