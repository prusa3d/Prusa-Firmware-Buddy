add_library(lwesp INTERFACE)

target_sources(lwesp INTERFACE
  src/api/lwesp_netconn.c
  src/lwesp/lwesp.c
  src/lwesp/lwesp_ap.c
  src/lwesp/lwesp_buff.c
  src/lwesp/lwesp_cli.c
  src/lwesp/lwesp_conn.c
  src/lwesp/lwesp_debug.c
  src/lwesp/lwesp_dhcp.c
  src/lwesp/lwesp_dns.c
  src/lwesp/lwesp_evt.c
  src/lwesp/lwesp_hostname.c
  src/lwesp/lwesp_input.c
  src/lwesp/lwesp_int.c
  src/lwesp/lwesp_mdns.c
  src/lwesp/lwesp_mem.c
  src/lwesp/lwesp_parser.c
  src/lwesp/lwesp_pbuf.c
  src/lwesp/lwesp_ping.c
  src/lwesp/lwesp_smart.c
  src/lwesp/lwesp_sntp.c
  src/lwesp/lwesp_sta.c
  src/lwesp/lwesp_threads.c
  src/lwesp/lwesp_timeout.c
  src/lwesp/lwesp_unicode.c
  src/lwesp/lwesp_utils.c
  src/lwesp/lwesp_wps.c

  src/system/lwesp_hal_stm32.c
  src/system/lwesp_mem_lwmem.c
  src/system/lwesp_sys_freertos.c
)

target_include_directories(
  lwesp
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    src/include
    src/include/system/port/freertos
)

target_link_libraries(
  lwesp
  INTERFACE WUI
)
