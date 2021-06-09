add_library(WUI INTERFACE)

target_sources(
  WUI
  INTERFACE wui.c
            wui_REST_api.c
            http/httpd.c
            lwip.c
            ethernetif.c
            netif_settings.c
            sntp/sntp_client.c
            sntp/sntp.c
            buddy_socket.c
            lan_interface.c
            cyclone-tcp/common/os_port_freertos.c
            cyclone-tcp/common/path.c
            cyclone-tcp/common/resource_manager.c
            cyclone-tcp/common/str.c
            resources/res.c
            ${CMAKE_SOURCE_DIR}/lib/CycloneTCP/http/http_server.c
            ${CMAKE_SOURCE_DIR}/lib/CycloneTCP/http/http_server_auth.c
            ${CMAKE_SOURCE_DIR}/lib/CycloneTCP/http/http_server_misc.c
            ${CMAKE_SOURCE_DIR}/lib/CycloneTCP/http/mime.c
  )

target_include_directories(
  WUI
  INTERFACE ${CMAKE_CURRENT_LIST_DIR}
            http
            sntp
            resources
            ${CMAKE_SOURCE_DIR}/lib/CycloneTCP
            cyclone-tcp/common
            cyclone-tcp/core
  )
target_link_libraries(WUI INTERFACE LwIP)
