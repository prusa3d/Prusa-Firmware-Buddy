# run from linux shell: $ ctest -S build.bitsery.cmake
set(CTEST_SOURCE_DIRECTORY "..") # path to bitsery root (top-level) directory
set(CTEST_BINARY_DIRECTORY "build")

set(ENV{CXXFLAGS} "--coverage")
# when using Ninja generator, ctest_coverage cannot find files...
set(CTEST_CMAKE_GENERATOR "CodeBlocks - Unix Makefiles")

set(CTEST_COVERAGE_COMMAND "gcov")

configure_file(CTestConfig.cmake ${CTEST_SOURCE_DIRECTORY}/CTestConfig.cmake)

ctest_start("Continuous")
ctest_configure(OPTIONS "-DBITSERY_BUILD_EXAMPLES=OFF;-DBITSERY_BUILD_TESTS=ON")
ctest_build()
ctest_test()
ctest_coverage()
# ctest_submit()
