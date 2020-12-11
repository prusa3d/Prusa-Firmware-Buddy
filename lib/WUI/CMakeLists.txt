add_library(WUI INTERFACE)

target_sources(WUI INTERFACE
  wui.c
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
)

add_custom_command(
  OUTPUT "${PROJECT_ROOT_DIR}/lib/WUI/resources/fsdata_wui_local.c"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/wui.py"
  COMMENT "Generating fsdata_wui_local raw data from WUI resources."
  VERBATIM
  )

target_include_directories(
  WUI
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    http
    sntp
    resources
)

target_link_libraries(
  WUI
  INTERFACE LwIP
  )
