add_library(WUI INTERFACE)

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
            $<$<BOOL:${WUI_ESP_DMA_TEST}>:${CMAKE_CURRENT_LIST_DIR}/tests/at_link.c>
  )

target_include_directories(
  WUI INTERFACE ${CMAKE_CURRENT_LIST_DIR} http sntp resources esp
                $<$<BOOL:${WUI_ESP_DMA_TEST}>:${CMAKE_CURRENT_LIST_DIR}/tests>
  )

target_link_libraries(WUI INTERFACE LwIP)
