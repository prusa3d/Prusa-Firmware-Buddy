cmake_minimum_required(VERSION 3.15)

project(
  Buddy
  LANGUAGES C ASM
  VERSION 1.0.0
  )

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
            prusa/src/system/lwesp_sys_buddy.c
            prusa/src/system/lwesp_ll_buddy.c
  )

target_include_directories(
  lwesp
  INTERFACE lwesp/lwesp/src/include
            prusa/include/system/port/buddy
            ${CMAKE_SOURCE_DIR}/include/esp
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/include
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
            ${CMAKE_SOURCE_DIR}/include/freertos
            lwesp/lwesp
            lwesp/snippets/include
  )

add_library(flasher INTERFACE)

target_sources(
  flasher INTERFACE serial-flasher/src/esp_loader.c serial-flasher/src/esp_targets.c
                    prusa/src/serial-flasher/serial_comm.c prusa/src/serial-flasher/buddy_port.c
  )

target_include_directories(
  flasher INTERFACE serial-flasher/include serial-flasher/port serial-flasher/private_include
  )
