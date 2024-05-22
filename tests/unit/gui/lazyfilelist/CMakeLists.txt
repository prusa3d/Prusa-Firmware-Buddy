# define the test executable
add_executable(
  lazyfilelist_tests
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ourPosix.cpp
  tests.cpp
  missing_functions.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/file_sort.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/lazyfilelist.cpp
  )

# define required search paths
target_include_directories(
  lazyfilelist_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui ${CMAKE_SOURCE_DIR}/src/guiapi/include
                            ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src
  )

# define custom macros
target_compile_definitions(lazyfilelist_tests PUBLIC LAZYFILELIST_UNITTEST)

# tell build system about the test case
add_catch_test(lazyfilelist_tests)
