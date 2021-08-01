cmake_minimum_required(VERSION 3.15)

project(
  Buddy
  LANGUAGES C ASM
  VERSION 1.0.0
  )

add_library(lwesp INTERFACE)

target_sources(
  lwesp
  INTERFACE lwesp/src/esp/esp.c
            lwesp/src/esp/esp_ap.c
            lwesp/src/esp/esp_buff.c
            lwesp/src/esp/esp_cli.c
            lwesp/src/esp/esp_conn.c
            # lwesp/src/esp/esp_dhcp.c lwesp/src/esp/esp_dns.c
            lwesp/src/esp/esp_evt.c
            lwesp/src/esp/esp_hostname.c
            lwesp/src/esp/esp_input.c
            lwesp/src/esp/esp_int.c
            lwesp/src/esp/esp_mdns.c
            lwesp/src/esp/esp_mem.c
            lwesp/src/esp/esp_parser.c
            lwesp/src/esp/esp_pbuf.c
            lwesp/src/esp/esp_ping.c
            # lwesp/src/esp/esp_smart.c
            lwesp/src/esp/esp_sntp.c
            lwesp/src/esp/esp_sta.c
            lwesp/src/esp/esp_threads.c
            lwesp/src/esp/esp_timeout.c
            lwesp/src/esp/esp_unicode.c
            # lwesp/src/esp/esp_utils.c
            lwesp/src/esp/esp_wps.c
            # lwesp/src/system/esp_mem_lwmem.c
            lwesp/src/apps/http_server/esp_http_server.c
            lwesp/src/apps/http_server/esp_http_server_fs.c
            prusa/src/system/lwesp_sys_buddy.c
            prusa/src/system/lwesp_ll_buddy.c
            prusa/src/esp.c
            prusa/src/system/esp_tcp.c
            prusa/snippets/client.c
            lwesp/src/api/esp_netconn.c
            prusa/src/sockets/lwesp_sockets.c
            prusa/src/sockets/lwesp_netbuf.c
  )

target_include_directories(
  lwesp
  INTERFACE lwesp/src/include
            prusa/include/system/port/buddy
            ${CMAKE_SOURCE_DIR}/include/esp
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/include
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
            ${CMAKE_SOURCE_DIR}/include/freertos
            lwesp/snippets/include
            prusa/include
  )

add_library(flasher INTERFACE)

target_sources(
  flasher INTERFACE serial-flasher/src/esp_loader.c serial-flasher/src/esp_targets.c
                    prusa/src/serial-flasher/serial_comm.c prusa/src/serial-flasher/buddy_port.c
  )

target_include_directories(
  flasher INTERFACE serial-flasher/include serial-flasher/port serial-flasher/private_include
  )
