add_executable(
  xbuddy_extension_tests
  ${CMAKE_SOURCE_DIR}/src/common/feature/xbuddy_extension/cooling.cpp
  ${CMAKE_SOURCE_DIR}/tests/unit/common/feature/xbuddy_extension/config_store/store_instance.cpp
  cooling_tests.cpp
  )
target_include_directories(
  xbuddy_extension_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/common
                                 ${CMAKE_SOURCE_DIR}/tests/unit/common/feature/xbuddy_extension
  )
add_catch_test(xbuddy_extension_tests)
