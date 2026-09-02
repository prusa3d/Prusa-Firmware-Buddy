add_executable(
  signal_processing_tests analysis_testing.cpp cholesky_testing.cpp filter_testing.cpp
                          pipeline_testing.cpp
  )

target_link_libraries(signal_processing_tests signal_processing)
add_catch_test(signal_processing_tests)
