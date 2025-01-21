# define the test executable
add_executable(probe_analysis_tests ${CMAKE_SOURCE_DIR}/src/common/probe_analysis.cpp tests.cpp)

# define required search paths
target_include_directories(
  probe_analysis_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/include
  )

# tell build system about the test case
add_catch_test(probe_analysis_tests)

# define the test executable
add_executable(
  probe_position_lookback ${CMAKE_SOURCE_DIR}/src/common/probe_position_lookback.cpp
                          probe_position_lookback_test.cpp
  )

# define required search paths
target_include_directories(
  probe_position_lookback PUBLIC . ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/include
                                 ${CMAKE_SOURCE_DIR}/lib/SG14
  )

# tell build system about the test case
add_catch_test(probe_position_lookback)
