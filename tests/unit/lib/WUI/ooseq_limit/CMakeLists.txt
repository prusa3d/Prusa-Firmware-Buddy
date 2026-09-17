add_executable(ooseq_limit_tests ooseq_limit_test.cpp)

target_include_directories(ooseq_limit_tests PRIVATE ${CMAKE_SOURCE_DIR}/lib/WUI)

add_catch_test(ooseq_limit_tests)
