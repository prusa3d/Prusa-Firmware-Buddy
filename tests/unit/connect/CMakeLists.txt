add_executable(
  connect_tests
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/types.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.c
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/httpc.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/resp_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/render.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/command.cpp
  ${CMAKE_SOURCE_DIR}/src/Connect/planner.cpp
  command.cpp
  http_client.cpp
  missing_functions.cpp
  render.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/jsmn_impl.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  )
target_include_directories(
  connect_tests PRIVATE . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/Connect
                        ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/lib/jsmn
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

# Planner tests are separate. They mock time and don't want to poison that for others.
add_executable(
  connect_planner_tests ${CMAKE_SOURCE_DIR}/src/Connect/planner.cpp time_mock.cpp planner.cpp
  )
target_include_directories(
  connect_planner_tests PRIVATE . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/Connect
                                ${CMAKE_SOURCE_DIR}/src/common
  )
add_definitions(-DUNITTESTS)

add_catch_test(connect_tests)
add_catch_test(connect_planner_tests)
