# first, let's create a symbolic target for all tests
add_custom_target(tests)

add_compile_definitions(UNITTEST)

# include catch_discover_tests function from Catch2
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

add_library(catch_main ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp)
target_link_libraries(catch_main Catch2::Catch2WithMain)

# registers given target as a catch test
function(add_catch_test target)
  target_link_libraries(${target} catch_main Catch2::Catch2WithMain)
  catch_discover_tests(${target})
  add_dependencies(tests ${target})
endfunction()

add_executable(system_tests ${CMAKE_CURRENT_SOURCE_DIR}/system_test.cpp)
add_catch_test(system_tests)

set(MODULES_STUBS_DIR ${CMAKE_SOURCE_DIR}/tests/unit/modules/stubs)
set(LOGIC_STUBS_DIR ${CMAKE_SOURCE_DIR}/tests/unit/logic/stubs)

# now, include all the unit tests; they should add themselves using the add_catch_test function
add_subdirectory(hal)
add_subdirectory(logic)
add_subdirectory(modules)

add_subdirectory(application)
