add_executable(
  nhttp_tests
  # Not really tests, just stuffings to satisfy the linker.
  ${CMAKE_CURRENT_SOURCE_DIR}/missing_functions.c
  ${CMAKE_CURRENT_SOURCE_DIR}/random_mock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/time_mock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/status_renderer_mock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partial_file_mock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/transfer_mock.cpp
  # Some handlers are stubbed out.
  ${CMAKE_CURRENT_SOURCE_DIR}/server_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/req_parser_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/job_command_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/file_command_tests.cpp
  # ${CMAKE_CURRENT_SOURCE_DIR}/segmented_json_rendering.cpp
  # Tested files
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/common_selectors.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/file_info.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/file_command.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/gcode_upload.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/gcode_preview.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/server.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/job_command.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/static_mem.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/splice.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/send_file.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/headers.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/status_page.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/send_json.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/transfer_renderer.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/req_parser.cpp
  ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp/handler.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/common/json_encode.cpp
  ${CMAKE_SOURCE_DIR}/src/common/segmented_json.cpp
  ${CMAKE_SOURCE_DIR}/src/common/automata/core.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/common/base64_stream_decoder.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filepath_operation.cpp
  ${CMAKE_SOURCE_DIR}/src/common/http/url_decode.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_any.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_binary.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_interface.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_plaintext.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_buffer.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/meatpack.cpp
  ${CMAKE_SOURCE_DIR}/src/common/stat_retry.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/changed_path.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/files.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/transfer_recovery.cpp
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/sha256.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/md5.c
  ${CMAKE_SOURCE_DIR}/lib/heatshrink/heatshrink_decoder.c
  # Dependencies.
  # It seems it's not possible to just link the damn LwIP library:
  # * Somehow, the target_link_libraries directive poisons include
  # paths, not only for us, but for other unit tests too...
  # * We need to run LwIP on top of "ordinary" OS, not FreeRTOS, so
  # parts need to be stubbed or reconfigured.
  #
  # It still seems there should be some better solution than just
  # keeping on adding the files in here until all undefined reference
  # errors disappear.
  #
  # Also, part of that is stubbed in missing_functions.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/altcp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/altcp_tcp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/def.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/pbuf.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ip.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_addr.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/icmp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/etharp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_frag.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/udp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/dns.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp_in.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp_out.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/timeouts.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/memp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/netif.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/netif/ethernet.c
  ${CMAKE_SOURCE_DIR}/tests/unit/common/printer_state_mock.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/jsmn_impl.c
  ${CMAKE_SOURCE_DIR}/tests/unit/mock/bsod.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
  )
target_include_directories(
  nhttp_tests
  PRIVATE .
          ${CMAKE_CURRENT_SOURCE_DIR}/mock
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_BINARY_DIR}/include
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/lib/WUI
          ${CMAKE_SOURCE_DIR}/lib/WUI/http
          ${CMAKE_SOURCE_DIR}/lib/WUI/nhttp
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src/common/http
          ${CMAKE_SOURCE_DIR}/src/common/gcode
          ${CMAKE_SOURCE_DIR}/src/persistent_stores
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/include
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/include
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/system
          ${CMAKE_SOURCE_DIR}/lib/jsmn
          ${CMAKE_SOURCE_DIR}/lib/heatshrink
          ${CMAKE_SOURCE_DIR}/lib
          ${CMAKE_SOURCE_DIR}/include/mbedtls
          ${CMAKE_SOURCE_DIR}/include/usb_host
  )

file(GLOB AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.h
  DEPENDS ${AUTOMATA_GENERATORS}
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_ROOT_DIR}/utils/gen-automata/http_server.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating http automata"
  )
add_custom_target(
  generate-http-automata-tests DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
                                       ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.h
  )
add_dependencies(nhttp_tests generate-http-automata-tests)
# target_link_libraries(http_tests LwIP)
set_target_properties(nhttp_tests PROPERTIES CXX_STANDARD 20)

target_link_libraries(nhttp_tests bgcode_core mbedTLS SG14 freertos_tests)

add_catch_test(nhttp_tests)
