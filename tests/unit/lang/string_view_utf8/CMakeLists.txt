# define the test executable
add_executable(string_view_utf8_tests tests.cpp)

# define required search paths
target_include_directories(string_view_utf8_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/lang)

# define custom macros

# target_compile_definitions(string_view_utf8_tests PUBLIC LAZYFILELIST_UNITTEST)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zluty_kun.bin ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

# tell build system about the test case
add_catch_test(string_view_utf8_tests)
