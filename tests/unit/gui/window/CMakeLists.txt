# define the test executable
add_executable(
  window_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_mocks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_windows.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/i_window_text_stub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/window_text_stub.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/knob_event.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window_frame.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/screen.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/display_ex.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/display_helper.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/IDialog.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/fonts.cpp
  )

add_executable(
  layout_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_mocks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_windows.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/knob_event.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window_frame.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/i_window_text.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window_text.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/window_icon_stub.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/screen.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/display_ex.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/display_helper.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/IDialog.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/DialogTimed.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  )

# define required search paths
target_include_directories(
  window_tests
  PUBLIC .
         ${CMAKE_SOURCE_DIR}/src/guiapi/include
         ${CMAKE_SOURCE_DIR}/src/guiconfig
         ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/common/marlin_server_types
         ${CMAKE_SOURCE_DIR}/src/gui/dialogs
         ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_BINARY_DIR}/include
         ${CMAKE_SOURCE_DIR}/src/common/utils
         ${CMAKE_SOURCE_DIR}/src/lang
  )

target_include_directories(
  layout_tests
  PUBLIC .
         ${CMAKE_SOURCE_DIR}/src/guiapi/include
         ${CMAKE_SOURCE_DIR}/src/guiconfig
         ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/common/utils
         ${CMAKE_SOURCE_DIR}/src/common/marlin_server_types
         ${CMAKE_SOURCE_DIR}/src/gui/dialogs
         ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_BINARY_DIR}/include
         ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_SOURCE_DIR}/src/lang
  )

target_link_libraries(window_tests SG14)

target_link_libraries(layout_tests SG14)

target_link_libraries(window_tests SG14)

target_link_libraries(layout_tests SG14)

target_link_libraries(window_tests SG14)

target_link_libraries(layout_tests SG14)

# tell build system about the test case
add_catch_test(window_tests)
add_catch_test(layout_tests)
