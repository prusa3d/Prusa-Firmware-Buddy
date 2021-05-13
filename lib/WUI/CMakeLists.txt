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
            ${CMAKE_SOURCE_DIR}/lib/SSL/prusa/hardware_rng.c
            ${CMAKE_SOURCE_DIR}/lib/SSL/prusa/net_sockets.c
            ${CMAKE_SOURCE_DIR}/lib/SSL/prusa/mbedtls_integration.c
            ${CMAKE_SOURCE_DIR}/lib/SSL/mbedtls/library/memory_buffer_alloc.c
  )

add_custom_target(
  mbedtls-release
  COMMAND
    ${CMAKE_COMMAND} -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} ${MCU_FLAGS} -DMBEDTLS_CONFIG_FILE=<${CMAKE_SOURCE_DIR}/include/mbedtls_config.h>"
    -DCMAKE_STATIC_LINKER_FLAGS_RELEASE="${CMAKE_STATIC_LINKER_FLAGS_RELEASE}"
    -DLIB_INSTALL_DIR=${CMAKE_BINARY_DIR}/lib -DENABLE_PROGRAMS=OFF -S
    ${CMAKE_SOURCE_DIR}/lib/SSL/mbedtls -B ${CMAKE_BINARY_DIR}/lib/mbedtls-release -G Ninja
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/lib/mbedtls-release --target mbedtls
  COMMENT "MbedTLS v2.26.0 relase build" USES_TERMINAL
  )

target_include_directories(
  WUI INTERFACE ${CMAKE_CURRENT_LIST_DIR} http sntp resources
                ${CMAKE_SOURCE_DIR}/lib/SSL/mbedtls/include
  )
target_link_directories(WUI INTERFACE ${CMAKE_BINARY_DIR}/lib/mbedtls-release/library)
target_link_libraries(WUI INTERFACE LwIP mbedtls mbedcrypto mbedx509)
