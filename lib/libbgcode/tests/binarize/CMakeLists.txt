add_executable(binarize_tests binarize_tests.cpp)

target_link_libraries(binarize_tests ${_libname}_binarize test_common)

catch_discover_tests(binarize_tests EXTRA_ARGS ${CATCH_EXTRA_ARGS})
