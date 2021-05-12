add_library(WUI INTERFACE)

add_dependencies(WUI mbedtls)

target_sources(
  WUI
  INTERFACE wui.c
            wui_REST_api.c
            http/altcp_proxyconnect.c
            http/fs.c
            http/httpd.c
            http/httpd_parser.c
            lwip.c
            ethernetif.c
            netif_settings.c
            sntp/sntp_client.c
            sntp/sntp.c
            ${CMAKE_SOURCE_DIR}/lib/SSL/prusa/hardware_rng.c
            ${CMAKE_SOURCE_DIR}/lib/SSL/mbedtls/library/memory_buffer_alloc.c
  )

target_include_directories(WUI INTERFACE ${CMAKE_CURRENT_LIST_DIR} http sntp resources)

target_link_libraries(WUI INTERFACE LwIP mbedtls)
