# first, let's create a symbolic target for all tests
add_custom_target(tests)

# include catch_discover_tests function from Catch2
include(${Catch2_SOURCE_DIR}/contrib/Catch.cmake)

add_library(catch_main ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp)
target_link_libraries(catch_main Catch2::Catch2)

# registers given target as a catch test
function(add_catch_test target)
  target_link_libraries(${target} catch_main Catch2::Catch2)
  catch_discover_tests(${target})
  add_dependencies(tests ${target})
endfunction()

add_executable(system_tests ${CMAKE_CURRENT_SOURCE_DIR}/system_test.cpp)
add_catch_test(system_tests)

# now, include all the unit tests; they should add themselves using the add_catch_test function
add_subdirectory(gui)
add_subdirectory(common)
add_subdirectory(lang)
add_subdirectory(lib)
add_subdirectory(logging)
