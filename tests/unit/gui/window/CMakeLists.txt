# define the test executable
add_executable(
  window_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_mocks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_windows.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/window_text_stub.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window_frame.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/screen.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/display_ex.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/window_dlg_popup.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/IDialog.cpp
  )

add_executable(
  layout_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_mocks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_windows.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/window_text_stub.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window_frame.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/screen.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/display_ex.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/window_dlg_popup.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/dialogs/IDialog.cpp
  )

# define required search paths
target_include_directories(window_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/guiapi/include)
target_include_directories(window_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)
target_include_directories(window_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui/dialogs)
target_include_directories(layout_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/guiapi/include)
target_include_directories(layout_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)
target_include_directories(layout_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui/dialogs)

# tell build system about the test case
add_catch_test(window_tests)
add_catch_test(layout_tests)
