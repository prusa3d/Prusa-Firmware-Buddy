add_library(lwesp INTERFACE)

target_sources(
  lwesp
  INTERFACE lwesp/lwesp/src/lwesp/lwesp.c
            lwesp/lwesp/src/lwesp/lwesp_ap.c
            lwesp/lwesp/src/lwesp/lwesp_buff.c
            lwesp/lwesp/src/lwesp/lwesp_cli.c
            lwesp/lwesp/src/lwesp/lwesp_conn.c
            lwesp/lwesp/src/lwesp/lwesp_debug.c
            lwesp/lwesp/src/lwesp/lwesp_dhcp.c
            lwesp/lwesp/src/lwesp/lwesp_dns.c
            lwesp/lwesp/src/lwesp/lwesp_evt.c
            lwesp/lwesp/src/lwesp/lwesp_hostname.c
            lwesp/lwesp/src/lwesp/lwesp_input.c
            lwesp/lwesp/src/lwesp/lwesp_int.c
            lwesp/lwesp/src/lwesp/lwesp_mdns.c
            lwesp/lwesp/src/lwesp/lwesp_mem.c
            lwesp/lwesp/src/lwesp/lwesp_parser.c
            lwesp/lwesp/src/lwesp/lwesp_pbuf.c
            lwesp/lwesp/src/lwesp/lwesp_ping.c
            lwesp/lwesp/src/lwesp/lwesp_smart.c
            lwesp/lwesp/src/lwesp/lwesp_sntp.c
            lwesp/lwesp/src/lwesp/lwesp_sta.c
            lwesp/lwesp/src/lwesp/lwesp_threads.c
            lwesp/lwesp/src/lwesp/lwesp_timeout.c
            lwesp/lwesp/src/lwesp/lwesp_unicode.c
            lwesp/lwesp/src/lwesp/lwesp_utils.c
            lwesp/lwesp/src/lwesp/lwesp_wps.c
            lwesp/lwesp/src/system/lwesp_mem_lwmem.c
            lwesp/lwesp/src/system/lwesp_sys_buddy.c
            lwesp/lwesp/src/system/lwesp_ll_buddy.c
  )

target_include_directories(
  lwesp
  INTERFACE lwesp/lwesp/src/include
            lwesp/lwesp/src/include/system/port/buddy
            ${CMAKE_SOURCE_DIR}/include/esp
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/include
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
            ${CMAKE_SOURCE_DIR}/include/freertos
            lwesp/lwesp
  )
