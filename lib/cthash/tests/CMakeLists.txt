find_package(Catch2 3 REQUIRED)

file(
  GLOB_RECURSE TESTS_SOURCES LINK_DIRECTORIES false
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cpp"
  )

add_executable(test-runner ${TESTS_SOURCES})

option(MEASURE_OPENSSL "Measure also OpenSSL" ON)

if(MEASURE_OPENSSL)
  find_package(OpenSSL REQUIRED Crypto)
  target_link_libraries(test-runner PRIVATE OpenSSL::Crypto)
  target_compile_definitions(test-runner PRIVATE OPENSSL_BENCHMARK OPENSSL_SUPPRESS_DEPRECATED)
endif()

target_link_libraries(test-runner PRIVATE Catch2::Catch2WithMain cthash)
target_compile_features(test-runner PUBLIC cxx_std_20)

add_custom_target(
  test
  test-runner --skip-benchmarks --colour-mode ansi ""
  DEPENDS test-runner
  )
add_custom_target(
  long-test
  test-runner --skip-benchmarks --colour-mode ansi "*,[.long]"
  DEPENDS test-runner
  )
add_custom_target(
  benchmark
  test-runner --colour-mode ansi ""
  DEPENDS test-runner
  )

if(CTHASH_COVERAGE)
  coverage_report_after(test test-runner)
endif()
