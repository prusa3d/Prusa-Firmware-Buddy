# define the test executable
add_executable(probe_analysis_tests tests.cpp)

# define required search paths
target_include_directories(probe_analysis_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# tell build system about the test case
add_catch_test(probe_analysis_tests)

# define the test executable
add_executable(
  probe_position_lookback ${CMAKE_SOURCE_DIR}/src/common/probe_position_lookback.cpp
                          probe_position_lookback_test.cpp
  )

# define required search paths
target_include_directories(probe_position_lookback PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# tell build system about the test case
add_catch_test(probe_position_lookback)
