add_executable(binarize_tests
    binarize_tests.cpp
    decompress_hardening_tests.cpp
    empty_block_tests.cpp
    heatshrink_roundtrip_tests.cpp
    meatpack_encode_tests.cpp
    meatpack_overflow_tests.cpp
    metadata_iterator_tests.cpp
)

target_link_libraries(binarize_tests ${_libname}_binarize test_common)

catch_discover_tests(binarize_tests EXTRA_ARGS ${CATCH_EXTRA_ARGS})
