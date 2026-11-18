add_executable(fanctl_tests fanctl_tests.cpp)
target_link_libraries(fanctl_tests fanctl)
add_catch_test(fanctl_tests)
