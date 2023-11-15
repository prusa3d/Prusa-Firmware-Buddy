add_executable(
  common_http_tests
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/connection.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/httpc.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/resp_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/os_porting.cpp
  http_client.cpp
  )
target_include_directories(
  common_http_tests
  PRIVATE .
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_SOURCE_DIR}/src/connect
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/lib/jsmn
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/common/http
          ${CMAKE_SOURCE_DIR}/src/common/utils
  )

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
add_dependencies(common_http_tests generate-httpc-automata-tests)

add_catch_test(common_http_tests)
