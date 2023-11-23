add_executable(core_tests core_tests.cpp)

target_link_libraries(core_tests ${_libname}_core test_common)

catch_discover_tests(core_tests EXTRA_ARGS ${CATCH_EXTRA_ARGS})
