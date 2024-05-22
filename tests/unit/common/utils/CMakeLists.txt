# define the test executable
add_executable(center_approx center_approx.cpp)

target_include_directories(
  center_approx
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common/utils ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/core
         ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/inc
  )

# tell build system about the test case
add_catch_test(center_approx)
