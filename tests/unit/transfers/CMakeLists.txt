add_executable(
  transfers_tests
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/fake_freertos_mutex.cpp monitor.cpp missing_functions.cpp
  )
target_include_directories(
  transfers_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
  )
add_definitions(-DUNITTESTS)

file(GLOB AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

add_catch_test(transfers_tests)
