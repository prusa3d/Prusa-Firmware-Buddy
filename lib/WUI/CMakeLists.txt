add_library(WUI INTERFACE)

add_dependencies(WUI mbedtls-release)

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
            ${CMAKE_CURRENT_SOURCE_DIR}/../SSL/prusa/hardware_rng.c
            ${CMAKE_CURRENT_SOURCE_DIR}/../SSL/prusa/net_sockets.c
            ${CMAKE_CURRENT_SOURCE_DIR}/../SSL/prusa/mbedtls_integration.c
            ${CMAKE_CURRENT_SOURCE_DIR}/../SSL/mbedtls/library/memory_buffer_alloc.c
  )

list(
  APPEND
    MBED_COMPILE_OPTIONS
    "-DMBEDTLS_CONFIG_FILE=<${CMAKE_CURRENT_SOURCE_DIR}/../../include/mbedtls_config.h>;${MCU_FLAGS}"
  )
set(MBEDTLS_BINARY_DIR ${CMAKE_BINARY_DIR}/lib/mbedtls-release)

add_custom_target(
  mbedtls-release
  COMMAND
    ${CMAKE_COMMAND} -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_BUILD_TYPE=Release
    -DCUSTOM_COMPILE_OPTIONS="${MBED_COMPILE_OPTIONS}"
    -DCMAKE_STATIC_LINKER_FLAGS_RELEASE="${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" -DENABLE_PROGRAMS=OFF
    -DENABLE_TESTING=OFF -S ${CMAKE_CURRENT_SOURCE_DIR}/../SSL -B ${MBEDTLS_BINARY_DIR} -G Ninja
  COMMAND ${CMAKE_COMMAND} --build ${MBEDTLS_BINARY_DIR} --target mbedtls
  COMMENT "MbedTLS v2.26.0 relase build" USES_TERMINAL
  )

target_include_directories(
  WUI INTERFACE ${CMAKE_CURRENT_LIST_DIR} http sntp resources
                ${CMAKE_CURRENT_SOURCE_DIR}/../SSL/mbedtls/include
  )
target_link_directories(WUI INTERFACE ${MBEDTLS_BINARY_DIR}/mbedtls/library)
target_link_libraries(WUI INTERFACE LwIP mbedtls mbedcrypto mbedx509)
