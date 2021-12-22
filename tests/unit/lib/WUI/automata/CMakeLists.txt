add_executable(
  automata_tests
  # Test files
  ${CMAKE_BINARY_DIR}/until_comma.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/automata/core.cpp
  # These are generated to test the generator
  generated.cpp
  traversal.cpp
  # Tested files
  )

target_include_directories(automata_tests PRIVATE . ${CMAKE_SOURCE_DIR}/lib/WUI ${CMAKE_BINARY_DIR})

# Parts of the tests are generated (to test the generator)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/until_comma.cpp ${CMAKE_BINARY_DIR}/until_comma.h
  DEPENDS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_ROOT_DIR}/utils/gen-automata/tests.py
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Generating test automata"
  )
add_custom_target(
  generate-test-automata DEPENDS ${CMAKE_BINARY_DIR}/until_comma.cpp
                                 ${CMAKE_BINARY_DIR}/until_comma.h
  )
add_dependencies(automata_tests generate-test-automata)

add_catch_test(automata_tests)
