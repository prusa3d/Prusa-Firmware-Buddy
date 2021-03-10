# define the test executable
add_executable(wui_esp_tests
${CMAKE_SOURCE_DIR}/tests/stubs/esp_sys_prusa3d.cpp
../../../../lib/WUI/esp/esp_parser.c
../../../../lib/WUI/esp/esp_int.c
../../../../lib/WUI/esp/esp_utils.c
../../../../lib/WUI/esp/esp_ap.c
../../../../lib/WUI/esp/esp_sta.c
../../test_main.cpp
tests.cpp
)

# define required search paths
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/lib/WUI)
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/lib/WUI/esp)
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# define custom macros
target_compile_definitions(wui_esp_tests PUBLIC ESP_UNITTEST)

# tell build system about the test case
add_catch_test(wui_esp_tests)
