add_executable(
  connect_tests
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.cpp
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filepath_operation.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_thumb_decoder.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/common/base64_stream_decoder.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/resp_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/httpc.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/socket.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/connection.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/socket_connection_factory.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/background.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/render.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/command.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/planner.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/printer.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/hostname.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/sleep.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/decrypt.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/download.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/files.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/changed_path.cpp
  command.cpp
  missing_functions.cpp
  missing_functions_time.cpp
  render.cpp
  hostname.cpp
  printer.cpp
  ${CMAKE_SOURCE_DIR}/tests/unit/common/printer_state_mock.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/jsmn_impl.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/fake_freertos_mutex.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/printer_type.cpp
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aes.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aesni.c
  )
target_include_directories(
  connect_tests
  PRIVATE .
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_BINARY_DIR}/include
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/connect
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src/common/http
          ${CMAKE_SOURCE_DIR}/src/common/gcode
          ${CMAKE_SOURCE_DIR}/src/persistent_stores
          ${CMAKE_SOURCE_DIR}/lib/jsmn
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/include
  )
add_definitions(-DUNITTESTS -DPRINTER_VERSION=3 -DPRINTER_SUBVERSION=0)

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
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filepath_operation.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/resp_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/httpc.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/socket.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/connection.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/socket_connection_factory.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/background.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/planner.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/command.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/printer.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/sleep.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/decrypt.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/download.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/files.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/changed_path.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/jsmn_impl.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/fake_freertos_mutex.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/printer_type.cpp
  time_mock.cpp
  missing_functions.cpp
  planner.cpp
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aes.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aesni.c
  )
target_include_directories(
  connect_planner_tests
  PRIVATE .
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_BINARY_DIR}/include
          ${CMAKE_SOURCE_DIR}/src/connect
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/lib/jsmn
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/include
  )
add_definitions(-DUNITTESTS)
add_dependencies(connect_planner_tests generate-httpc-automata-tests-connect)

add_executable(
  connect_registration_tests
  ${CMAKE_CURRENT_BINARY_DIR}/http_resp_automaton.cpp
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aes.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aesni.c
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/connection.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/httpc.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/resp_parser.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/socket.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/socket_connection_factory.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filepath_operation.cpp
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/background.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/json_out.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/planner.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/printer.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/registrator.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/sleep.cpp
  ${CMAKE_SOURCE_DIR}/src/connect/status.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/decrypt.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/download.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/files.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/changed_path.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/fake_freertos_mutex.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/printer_type.cpp
  missing_functions.cpp
  registrator.cpp
  time_mock.cpp
  )
target_include_directories(
  connect_registration_tests
  PRIVATE .
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_BINARY_DIR}/include
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/include
          ${CMAKE_SOURCE_DIR}/tests/stubs
  )
add_dependencies(connect_registration_tests generate-httpc-automata-tests-connect)

add_catch_test(connect_tests)
add_catch_test(connect_planner_tests)
add_catch_test(connect_registration_tests)
