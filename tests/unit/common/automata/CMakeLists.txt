add_executable(
  automata_tests
  # Some files generated for the test (to test the generator)
  ${CMAKE_BINARY_DIR}/http.cpp
  ${CMAKE_BINARY_DIR}/http_method.cpp
  ${CMAKE_BINARY_DIR}/http_req.cpp
  ${CMAKE_BINARY_DIR}/until_comma.cpp
  # Tested files
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  # Test files
  generated.cpp
  traversal.cpp
  )

target_include_directories(
  automata_tests PRIVATE . ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_BINARY_DIR}
  )

file(GLOB TEST_AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

# Parts of the tests are generated (to test the generator)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/until_comma.cpp
         ${CMAKE_BINARY_DIR}/until_comma.h
         ${CMAKE_BINARY_DIR}/http.cpp
         ${CMAKE_BINARY_DIR}/http.h
         ${CMAKE_BINARY_DIR}/http_method.cpp
         ${CMAKE_BINARY_DIR}/http_method.h
         ${CMAKE_BINARY_DIR}/http_req.cpp
         ${CMAKE_BINARY_DIR}/http_req.h
  DEPENDS ${TEST_AUTOMATA_GENERATORS}
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_ROOT_DIR}/utils/gen-automata/tests.py
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Generating test automata"
  )
add_custom_target(
  generate-test-automata
  DEPENDS ${CMAKE_BINARY_DIR}/http.cpp
          ${CMAKE_BINARY_DIR}/http.h
          ${CMAKE_BINARY_DIR}/http_method.cpp
          ${CMAKE_BINARY_DIR}/http_method.h
          ${CMAKE_BINARY_DIR}/http_req.cpp
          ${CMAKE_BINARY_DIR}/http_req.h
          ${CMAKE_BINARY_DIR}/until_comma.cpp
          ${CMAKE_BINARY_DIR}/until_comma.h
  )
add_dependencies(automata_tests generate-test-automata)

add_catch_test(automata_tests)
