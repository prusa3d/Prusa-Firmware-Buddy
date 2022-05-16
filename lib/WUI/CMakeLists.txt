add_library(WUI INTERFACE)

target_sources(
  WUI
  INTERFACE wui.cpp
            netdev.c
            json_encode.c
            wui_api.cpp
            nhttp/multipart_parser.c
            nhttp/upload_state.cpp
            ethernetif.c
            espif.cpp
            sntp/sntp_client.c
            sntp/sntp.c
            automata/core.cpp
            http_lifetime.cpp
            nhttp/common_selectors.cpp
            nhttp/file_info.cpp
            nhttp/file_command.cpp
            nhttp/file_command_marlin.cpp
            nhttp/gcode_upload.cpp
            nhttp/gcode_preview.cpp
            nhttp/handler.cpp
            nhttp/headers.cpp
            nhttp/job_command.cpp
            nhttp/job_command_marlin.cpp
            nhttp/req_parser.cpp
            nhttp/send_file.cpp
            nhttp/segmented_json.cpp
            nhttp/server.cpp
            nhttp/stateless_json.cpp
            nhttp/static_mem.cpp
            nhttp/status_page.cpp
            link_content/basic_gets.cpp
            link_content/prusa_link_api.cpp
            link_content/static_file.cpp
            link_content/usb_files.cpp
            link_content/previews.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
  )

file(GLOB AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/http_req_automaton.h
  DEPENDS ${AUTOMATA_GENERATORS}
  COMMAND ${Python3_EXECUTABLE} ${PROJECT_ROOT_DIR}/utils/gen-automata/http.py
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
