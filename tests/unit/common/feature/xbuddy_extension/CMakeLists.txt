add_executable(
  xbuddy_extension_tests ${CMAKE_SOURCE_DIR}/src/common/feature/xbuddy_extension/cooling.cpp
                         cooling.cpp
  )
target_include_directories(xbuddy_extension_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
add_catch_test(xbuddy_extension_tests)
