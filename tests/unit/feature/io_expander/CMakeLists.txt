add_executable(wait_deadline_tests ${CMAKE_CURRENT_SOURCE_DIR}/wait_deadline_tests.cpp)
target_include_directories(wait_deadline_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/marlin_stubs)

add_catch_test(wait_deadline_tests)
