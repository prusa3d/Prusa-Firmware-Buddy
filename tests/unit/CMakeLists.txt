# Turn RTTI  on for tests (disabled for firmware, on for tests)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-frtti>)

# first, let's create a symbolic target for all tests
add_custom_target(tests)

# Define UNITTESTS macro everywhere
add_definitions(-DUNITTESTS)

# Prepend our stubbed libintl header to override the host's gettext
include_directories(${CMAKE_SOURCE_DIR}/tests/stubs/host)

# include catch_discover_tests function from Catch2
include(${Catch2_SOURCE_DIR}/contrib/Catch.cmake)
add_library(catch_main ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp)
target_link_libraries(catch_main Catch2::Catch2 CppStdExtensions)

# registers given target as a catch test
function(add_catch_test target)
  target_link_libraries(${target} catch_main Catch2::Catch2)
  catch_discover_tests(${target})
  add_dependencies(tests ${target})
endfunction()

add_executable(system_tests ${CMAKE_CURRENT_SOURCE_DIR}/system_test.cpp)
add_catch_test(system_tests)

# Define printer-type related defines (set up as MINI)
add_compile_definitions(
  PRINTER_VERSION=1
  PRINTER_SUBVERSION=0
  PRINTER_TYPE=2
  PRINTER_CODE=12
  PRINTER_MODEL="MINI"
  BOARD=1
  BOARD_VERSION_MAJOR=0
  BOARD_VERSION_MINOR=0
  BOARD_VERSION_PATCH=0
  )

# now, include all the unit tests; they should add themselves using the add_catch_test function
add_subdirectory(gui)
add_subdirectory(common)
add_subdirectory(lang)
add_subdirectory(lib)
add_subdirectory(logging)
add_subdirectory(connect)
add_subdirectory(mmu2)
add_subdirectory(mmu2-modbus)
add_subdirectory(puppies)
add_subdirectory(transfers)
add_subdirectory(persistent_stores)
add_subdirectory(media_prefetch)
add_subdirectory(puppy)
