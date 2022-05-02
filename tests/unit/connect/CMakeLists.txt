add_executable(
  connect_tests
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missing_functions.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/httpc.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/resp_parser.cpp
  http_client.cpp
  )
target_include_directories(
  connect_tests PRIVATE . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/Connect
                        ${CMAKE_SOURCE_DIR}/src/common
  )
add_definitions(-DUNITTESTS)

file(GLOB AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.h
  DEPENDS ${AUTOMATA_GENERATORS}
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_ROOT_DIR}/utils/gen-automata/http_client.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating http client automata"
  )
add_custom_target(
  generate-httpc-automata-tests DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
                                        ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.h
  )
add_dependencies(nhttp_tests generate-httpc-automata-tests)

add_catch_test(connect_tests)
