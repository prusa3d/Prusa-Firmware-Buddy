# define the test executable
add_executable(wui_esp_tests
${CMAKE_SOURCE_DIR}/tests/stubs/lwesp_sys_prusa3d.cpp
${CMAKE_SOURCE_DIR}/tests/stubs/lwesp_ll_prusa3d.cpp
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_parser.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_int.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_utils.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_ap.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_sta.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_mem.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_pbuf.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_conn.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_unicode.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_threads.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_evt.c
${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/lwesp/lwesp_timeout.c
tests.cpp
)

# define required search paths
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/lib/lwesp/lwesp/src/include)
target_include_directories(wui_esp_tests PUBLIC . ${CMAKE_SOURCE_DIR}/tests/stubs)

# tell build system about the test case
add_catch_test(wui_esp_tests)
