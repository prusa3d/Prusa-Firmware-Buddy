# define the test executable
add_executable(
  lazyfilelist_tests ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c ourPosix.cpp tests.cpp
                     ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  )

# define required search paths
target_include_directories(
  lazyfilelist_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui ${CMAKE_SOURCE_DIR}/src/guiapi/include
  )

# define custom macros
target_compile_definitions(lazyfilelist_tests PUBLIC LAZYFILELIST_UNITTEST)

# tell build system about the test case
add_catch_test(lazyfilelist_tests)
