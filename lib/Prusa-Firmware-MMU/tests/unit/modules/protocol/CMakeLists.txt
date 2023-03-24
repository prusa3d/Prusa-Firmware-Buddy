# define the test executable
add_executable(protocol_tests ${CMAKE_SOURCE_DIR}/src/modules/protocol.cpp test_protocol.cpp)

# define required search paths
target_include_directories(protocol_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules)

# tell build system about the test case
add_catch_test(protocol_tests)
