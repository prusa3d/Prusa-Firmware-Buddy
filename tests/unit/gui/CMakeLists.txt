# define the test executable
add_executable(
  rectangle_tests ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
                  ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp rectangle_tests.cpp
  )

add_executable(
  gui_time_tests
  ${CMAKE_SOURCE_DIR}/src/gui/gui_time.cpp ${CMAKE_CURRENT_SOURCE_DIR}/gui_time_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gui_time_tests.c
  )

# define required search paths
target_include_directories(
  rectangle_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/guiapi/include ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/guiconfig
         ${CMAKE_SOURCE_DIR}/include
  )
target_compile_definitions(
  rectangle_tests PUBLIC BOARD=3 BOARD_VERSION_MINOR=3 BOARD_VERSION_MAJOR=3 BOARD_VERSION_PATCH=3
  )

target_include_directories(
  gui_time_tests PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
                        ${CMAKE_SOURCE_DIR}/src/guiapi/include ${CMAKE_CURRENT_SOURCE_DIR}
  )
# tell build system about the test case
add_catch_test(rectangle_tests)
add_catch_test(gui_time_tests)

add_subdirectory(lazyfilelist)
add_subdirectory(jogwheel)
add_subdirectory(window)
add_subdirectory(neopixel)
