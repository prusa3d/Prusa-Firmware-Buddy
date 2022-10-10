add_executable(connect-dev-tests)

target_sources(
  connect-dev-tests
  PRIVATE ${CMAKE_SOURCE_DIR}/src/Connect/httpc.cpp ${CMAKE_SOURCE_DIR}/src/Connect/os_porting.cpp
          ${CMAKE_SOURCE_DIR}/src/Connect/socket.cpp dev-tests.cpp
  )

target_compile_options(connect-dev-tests PRIVATE -g -pthread -O0)
target_link_options(connect-dev-tests PRIVATE -g -O0)

target_link_libraries(connect-dev-tests PRIVATE pthread)

target_include_directories(
  connect-dev-tests PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_SOURCE_DIR}/src/Connect
  )
