# HAS_ESP() is a compile-time option and both settings ship, so the tests are
# built twice against the same sources: once without the wifi module (iX,
# XL_DEV_KIT) and once with it (MK4, MK3.5, XL, MINI, the Core Ones). The stub
# directory is what differs; it shadows the generated option header.
function(add_sntp_client_test target stub_dir)
  add_executable(
    ${target}
    # Not really tests, just stuffings to satisfy the linker.
    ${CMAKE_CURRENT_SOURCE_DIR}/missing_functions.c
    ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
    # The tests.
    ${CMAKE_CURRENT_SOURCE_DIR}/sntp_client_tests.cpp
    # Tested files.
    ${CMAKE_SOURCE_DIR}/lib/WUI/sntp/sntp.c
    ${CMAKE_SOURCE_DIR}/lib/WUI/sntp/sntp_client.c
    # LwIP dependencies, same approach as the nhttp tests.
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/def.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/pbuf.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ip.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_addr.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/icmp.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/etharp.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_frag.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/udp.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp_in.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp_out.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/dns.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/timeouts.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/memp.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/netif.c
    ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/netif/ethernet.c
    )
  target_include_directories(
    ${target}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${stub_dir}
            ${CMAKE_SOURCE_DIR}/tests/stubs # must be before ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/lib/WUI
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/include
            ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/system
    )
  target_link_libraries(${target} freertos_tests)

  add_catch_test(${target})
endfunction()

add_sntp_client_test(sntp_client_tests stub_noesp)
add_sntp_client_test(sntp_client_esp_tests stub_esp)
