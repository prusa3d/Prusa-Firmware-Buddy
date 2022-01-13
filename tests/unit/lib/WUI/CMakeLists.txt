add_executable(
  wui_utils_tests ${CMAKE_CURRENT_SOURCE_DIR}/json_encode_tests.cpp
                  ${CMAKE_SOURCE_DIR}/lib/WUI/json_encode.c
  )
target_include_directories(wui_utils_tests PRIVATE . ${CMAKE_SOURCE_DIR}/lib/WUI)

add_catch_test(wui_utils_tests)

add_subdirectory(automata)
add_subdirectory(nhttp)
