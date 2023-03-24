option(GCOV_ENABLE "Enable/use GCOV code coverage for tests" ON)

if (GCOV_ENABLE)
	find_program(GCOV_BINARY NAMES gcov gcov-7)
	find_program(LCOV_BINARY NAMES lcov)
	message(STATUS "Using gcov ${GCOV_BINARY} and LCOV ${LCOV_BINARY}")
	add_compile_options(-g -O0 -fprofile-arcs -ftest-coverage)
	link_libraries(-coverage -lgcov)
	set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)
	add_custom_target(tests_clean
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROJECT_BINARY_DIR}/Coverage
		COMMAND ${LCOV_BINARY} -b ${PROJECT_BINARY_DIR} -d ${PROJECT_BINARY_DIR} --zerocounters
		COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_BINARY_DIR}/.ctest-finished
	)

	# This step needs to always return OK but log whether it was successful or not. The thought here is that if the tests all
	# pass, .ctest-finished is created and we can check for its existance after generating the report to determine if the overall
	# build result is a pass or fail. 
	add_custom_target(test_run_all
		COMMAND cd ${PROJECT_BINARY_DIR} 
		COMMAND env CTEST_OUTPUT_ON_FAILURE=1 ctest --timeout 30 && ${CMAKE_COMMAND} -E touch .ctest-finished || exit 0
		DEPENDS tests_clean tests
	)

	add_custom_target(test_coverage_report
		COMMAND cd ${PROJECT_BINARY_DIR}
		COMMAND ${LCOV_BINARY} --capture --gcov-tool="${GCOV_BINARY}" --directory ${PROJECT_BINARY_DIR} --output-file ${PROJECT_BINARY_DIR}/coverage.info
		# Strip system headers and other uninteresting stuff. 
		COMMAND ${LCOV_BINARY} --remove coverage.info '/usr/*' '${PROJECT_SOURCE_DIR}/tests/*' '${PROJECT_SOURCE_DIR}/lib/Catch2/*' -o coverage.info
		# Package it up. 
		COMMAND genhtml coverage.info --output-directory Coverage
		COMMAND tar -zcvf Coverage.tar.gz Coverage
		# Cheat and compare a file to itself to check for existence. File-Not-Found is a failure code. 
		COMMAND ../../utils/gcovr.py -r . -e '../../tests' -e '../../lib/Catch2' | tee Summary.txt
		COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_BINARY_DIR}/.ctest-finished ${PROJECT_BINARY_DIR}/.ctest-finished
		DEPENDS test_run_all
	)
else()
	set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE OFF)
endif()

add_subdirectory(unit)
