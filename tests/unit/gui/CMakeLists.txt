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

add_executable(
  gui_tests ${CMAKE_CURRENT_SOURCE_DIR}/gui_tests.cpp
            ${CMAKE_SOURCE_DIR}/src/guiapi/src/numeric_input_config.cpp
  )

# define required search paths
target_include_directories(
  rectangle_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/guiapi/include ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/guiconfig
         ${CMAKE_SOURCE_DIR}/include
  )

target_include_directories(
  gui_time_tests PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
                        ${CMAKE_SOURCE_DIR}/src/guiapi/include ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_include_directories(
  gui_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/lang
         ${CMAKE_SOURCE_DIR}/src/guiapi/include ${CMAKE_CURRENT_SOURCE_DIR}
  )

# tell build system about the test case
add_catch_test(rectangle_tests)
add_catch_test(gui_time_tests)
add_catch_test(gui_tests)

# std_text_input_layout_tests
add_executable(std_text_input_layout_tests ${CMAKE_CURRENT_SOURCE_DIR}/text_input_layout_tests.cpp)
target_include_directories(
  std_text_input_layout_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/src/gui/dialogs ${CMAKE_SOURCE_DIR}/include
  )
target_compile_definitions(std_text_input_layout_tests PUBLIC BOARD=2)
add_catch_test(std_text_input_layout_tests)

# mini_text_input_layout_tests
add_executable(mini_text_input_layout_tests ${CMAKE_CURRENT_SOURCE_DIR}/text_input_layout_tests.cpp)
target_include_directories(
  mini_text_input_layout_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/src/gui/dialogs ${CMAKE_SOURCE_DIR}/include
  )
target_compile_definitions(mini_text_input_layout_tests PUBLIC BOARD=1)
add_catch_test(mini_text_input_layout_tests)

# color_tests
add_executable(
  color_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/color_tests.cpp ${CMAKE_SOURCE_DIR}/src/common/utils/color.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  )
target_include_directories(
  color_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src/lang
  )
add_catch_test(color_tests)

add_subdirectory(lazyfilelist)
add_subdirectory(jogwheel)
add_subdirectory(window)
add_subdirectory(neopixel)
