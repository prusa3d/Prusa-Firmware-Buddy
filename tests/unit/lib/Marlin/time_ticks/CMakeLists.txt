add_executable(time_ticks_tests ${CMAKE_CURRENT_SOURCE_DIR}/time_ticks_tests.cpp)
target_include_directories(time_ticks_tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/core)
add_compile_definitions(UNITTEST)
add_catch_test(time_ticks_tests)
