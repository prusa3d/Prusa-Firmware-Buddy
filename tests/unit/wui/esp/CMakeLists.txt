# define the test executable
add_executable(wui_esp_tests
../../test_main.cpp
tests.cpp
)

# define required search paths
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/lib/WUI)
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# define custom macros
target_compile_definitions(wui_esp_tests PUBLIC ESP_UNITTEST)

# tell build system about the test case
add_catch_test(wui_esp_tests)
