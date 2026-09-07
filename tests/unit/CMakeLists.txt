# Turn RTTI  on for tests (disabled for firmware, on for tests)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-frtti>)

# first, let's create a symbolic target for all tests
add_custom_target(tests)

# Define UNITTESTS macro everywhere
add_definitions(-DUNITTESTS)

# Make tests_include available for all tests
include_directories(tests_include)
link_libraries(magic_enum)

# Add implementation that fails test on BSOD everywhere
add_library(bsod_catch STATIC "${CMAKE_CURRENT_SOURCE_DIR}/tests_include/bsod.cpp")
target_link_libraries(bsod_catch PUBLIC bsod)
target_link_libraries(bsod_catch PRIVATE Catch2::Catch2)
link_libraries(bsod_catch)
# Link pthread for all tests (needed for some gcc versions)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# Prepend our stubbed libintl header to override the host's gettext
include_directories(${CMAKE_SOURCE_DIR}/tests/stubs/host)
# Global stubs for generated headers (e.g. option/signature_oak.h needed by color.hpp). color.hpp is
# transitively included by many unit tests and the codebase is too tightly coupled to stub per-test.
include_directories(${CMAKE_SOURCE_DIR}/tests/stubs/global)

# include catch_discover_tests function from Catch2
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
add_library(catch_main ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp)
target_link_libraries(catch_main PRIVATE Catch2::Catch2WithMain)

# registers given target as a catch test
function(add_catch_test target)
  target_link_libraries(${target} catch_main Catch2::Catch2)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/tests/unit/include)
  # Universal BSOD implementation that fails the tests
  catch_discover_tests(${target} ADD_TAGS_AS_LABELS)
  add_dependencies(tests ${target})
endfunction()

add_executable(system_tests ${CMAKE_CURRENT_SOURCE_DIR}/system_test.cpp)
add_catch_test(system_tests)

# Define printer-type related defines (set up as MINI)
add_compile_definitions(
  PRINTER_VERSION=1 PRINTER_SUBVERSION=0 PRINTER_TYPE=2 PRINTER_CODE=12 BOARD=BOARD_BUDDY
  )

# now, include all the unit tests; they should add themselves using the add_catch_test function
add_subdirectory(gui)
add_subdirectory(common)
add_subdirectory(lang)
add_subdirectory(lib)
add_subdirectory(logging)
add_subdirectory(connect)
add_subdirectory(marlin)
add_subdirectory(mmu2)
add_subdirectory(mmu2-modbus)
add_subdirectory(puppies)
add_subdirectory(resources)
add_subdirectory(transfers)
add_subdirectory(persistent_stores)
add_subdirectory(media_prefetch)
add_subdirectory(puppy)
add_subdirectory(module)
add_subdirectory(mock)
add_subdirectory(feature)
add_subdirectory(hw)
