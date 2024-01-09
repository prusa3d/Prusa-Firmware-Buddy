target_sources(
  firmware
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
          buffered_conn.cpp
          connect_error.cpp
          connection.cpp
          httpc.cpp
          os_porting.cpp
          post_file_request.cpp
          resp_parser.cpp
          socket.cpp
          socket_connection_factory.cpp
          url_decode.cpp
          websocket.cpp
  )

file(GLOB AUTOMATA_GENERATORS ${PROJECT_SOURCE_DIR}/utils/gen-automata/*.py)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.h
  DEPENDS ${AUTOMATA_GENERATORS}
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/utils/gen-automata/http_client.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating http client automata"
  )
add_custom_target(
  generate-httpc-automata DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
                                  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.h
  )
add_dependencies(firmware generate-httpc-automata)

target_include_directories(firmware PRIVATE . "${CMAKE_CURRENT_BINARY_DIR}")
