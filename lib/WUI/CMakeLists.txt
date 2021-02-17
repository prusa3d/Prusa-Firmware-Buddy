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
  #lwesp------
  lwesp/src/system/lwesp_sys_freertos_os.c
  lwesp/src/system/lwesp_mem_lwmem.c
  lwesp/src/system/lwesp_ll_stm32.c
  lwesp/src/system/lwesp_mem_lwmem.c
)

target_include_directories(
  WUI
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    http
    sntp
    resources
    lwesp/src/include
    lwesp/src/include/system/port/freertos
)

target_link_libraries(
  WUI
  INTERFACE LwIP
  )
