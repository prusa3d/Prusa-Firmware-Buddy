add_executable(tpis_tests tpis_tests.cpp)

target_link_libraries(tpis_tests tpis i2c_mock)
add_catch_test(tpis_tests)
