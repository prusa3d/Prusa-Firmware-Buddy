add_executable(
  connect_tests
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.c
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  ${CMAKE_SOURCE_DIR}/src/common/basename.c
  ${CMAKE_SOURCE_DIR}/src/common/gcode_thumb_decoder.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode_filename.cpp
  ${CMAKE_SOURCE_DIR}/src/common/base64_stream_decoder.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/resp_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/render.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/command.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/planner.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/hostname.cpp
  command.cpp
  missing_functions.cpp
  render.cpp
  hostname.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/jsmn_impl.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  )
target_include_directories(
  connect_tests
  PRIVATE . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/connect
          ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/lib/jsmn ${CMAKE_SOURCE_DIR}/src
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
  generate-httpc-automata-tests-connect DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
                                                ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.h
  )
add_dependencies(connect_tests generate-httpc-automata-tests-connect)

# Planner tests are separate. They mock time and don't want to poison that for others.
add_executable(
  connect_planner_tests
  ${CMAKE_SOURCE_DIR}/src/connect/planner.cpp ${CMAKE_SOURCE_DIR}/src/connect/command.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/jsmn_impl.c ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  time_mock.cpp planner.cpp
  )
target_include_directories(
  connect_planner_tests
  PRIVATE . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/connect
          ${CMAKE_SOURCE_DIR}/src/common ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/lib/jsmn
  )
add_definitions(-DUNITTESTS)

add_catch_test(connect_tests)
add_catch_test(connect_planner_tests)
