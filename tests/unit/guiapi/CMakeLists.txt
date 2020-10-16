
add_executable(window_menu_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/window_menu_tests.cpp
)
target_include_directories(window_menu_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/src/guiapi/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
)

add_catch_test(window_menu_tests)
