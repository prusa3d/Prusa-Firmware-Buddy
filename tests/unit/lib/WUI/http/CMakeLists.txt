add_executable(
  http_tests
  # Test files
  ${CMAKE_CURRENT_SOURCE_DIR}/http_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/multipart_upload.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missing_functions.c
  # Tested files
  ${CMAKE_SOURCE_DIR}/lib/WUI/http/handler.c
  ${CMAKE_SOURCE_DIR}/lib/WUI/http/httpd.c
  ${CMAKE_SOURCE_DIR}/lib/WUI/http/httpd_post.c
  ${CMAKE_SOURCE_DIR}/lib/WUI/http/fs.c
  ${CMAKE_SOURCE_DIR}/lib/WUI/http/multipart_parser.c
  ${CMAKE_SOURCE_DIR}/lib/WUI/http/upload_state.cpp
  # Dependencies.
  # It seems it's not possible to just link the damn LwIP library:
  # * Somehow, the target_link_libraries directive poisons include
  # paths, not only for us, but for other unit tests too...
  # * We need to run LwIP on top of "ordinary" OS, not FreeRTOS, so
  # parts need to be stubbed or reconfigured.
  #
  # It still seems there should be some better solution than just
  # keeping on adding the files in here until all undefined reference
  # errors disappear.
  #
  # Also, part of that is stubbed in missing_functions.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/altcp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/altcp_tcp.c
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
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/dns.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp_in.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/tcp_out.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/timeouts.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/memp.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/core/netif.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/netif/ethernet.c
  ${CMAKE_SOURCE_DIR}/src/lwip.c
  )
target_compile_definitions(http_tests PRIVATE -DHTTPD_FSDATA_FILE="fsdata_wui_local.c")
target_include_directories(
  http_tests
  PRIVATE .
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/lib/WUI
          ${CMAKE_SOURCE_DIR}/lib/WUI/http
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/include
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/system
  )
# target_link_libraries(http_tests LwIP)

add_catch_test(http_tests)
