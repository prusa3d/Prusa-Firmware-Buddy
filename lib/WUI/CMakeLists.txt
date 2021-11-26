add_library(WUI INTERFACE)

target_sources(
  WUI
  INTERFACE wui.c
            wui_REST_api.c
            netdev.c
            alsockets.c
            wui_api.c
            http/fs.c
            http/handler.c
            http_handler_default.c
            http/httpd.c
            http/httpd_post.c
            http/multipart_parser.c
            ethernetif.c
            sntp/sntp_client.c
            sntp/sntp.c
  )

target_include_directories(WUI INTERFACE ${CMAKE_CURRENT_LIST_DIR} http sntp resources esp)

target_link_libraries(WUI INTERFACE LwIP)
