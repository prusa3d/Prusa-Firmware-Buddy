add_library(WUI INTERFACE)

target_sources(
  WUI
  INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
            espif.cpp
            ethernetif.c
            ethernetif.cpp
            http_lifetime.cpp
            link_content/basic_gets.cpp
            link_content/previews.cpp
            link_content/prusa_api_helpers.cpp
            link_content/prusa_link_api_octo.cpp
            link_content/prusa_link_api_v1.cpp
            link_content/static_file.cpp
            link_content/usb_files.cpp
            netdev.c
            nhttp/common_selectors.cpp
            nhttp/file_command.cpp
            nhttp/file_command_marlin.cpp
            nhttp/file_info.cpp
            nhttp/gcode_preview.cpp
            nhttp/gcode_upload.cpp
            nhttp/handler.cpp
            nhttp/headers.cpp
            nhttp/job_command.cpp
            nhttp/job_command_marlin.cpp
            nhttp/req_parser.cpp
            nhttp/send_file.cpp
            nhttp/send_json.cpp
            nhttp/server.cpp
            nhttp/splice.cpp
            nhttp/static_mem.cpp
            nhttp/status_page.cpp
            nhttp/status_renderer.cpp
            nhttp/transfer_renderer.cpp
            pbuf_rx.cpp
            sntp/sntp.c
            sntp/sntp_client.c
            wui.cpp
            wui_api.cpp
  )

if(MDNS)
  target_sources(WUI INTERFACE mdns/mdns.c mdns/mdns_domain.c mdns/mdns_out.c)
endif()

if(NETWORKING_BENCHMARK_ENABLED)
  target_sources(
    WUI INTERFACE nhttp/networking_benchmark.cpp nhttp/networking_benchmark_selector.cpp
    )
  target_compile_definitions(WUI INTERFACE NETWORKING_BENCHMARK_ENABLED)
endif()

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
  generate-http-automata DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
                                 ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.h
  )
add_dependencies(WUI generate-http-automata generate-web-file)

target_include_directories(
  WUI
  INTERFACE
  INTERFACE ${CMAKE_SOURCE_DIR}/lib/WUI ${CMAKE_SOURCE_DIR}/lib/WUI/http
            ${CMAKE_SOURCE_DIR}/lib/WUI/sntp ${CMAKE_SOURCE_DIR}/lib/WUI/resources
            ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  )

target_link_libraries(WUI INTERFACE LwIP)
