# define the test executable
add_executable(
  jogwheel_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests_jogwheel.cpp
                 ${CMAKE_SOURCE_DIR}/src/guiapi/src/Jogwheel.cpp
  )

# define required search paths
target_include_directories(
  jogwheel_tests
  PRIVATE . ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/src/guiapi/include ${CMAKE_SOURCE_DIR}/tests/stubs
  )

# define custom macros
target_compile_definitions(jogwheel_tests PUBLIC WIZARD_UNITTEST)

target_link_libraries(jogwheel_tests SG14 freertos_tests)

# tell build system about the test case
add_catch_test(jogwheel_tests)
