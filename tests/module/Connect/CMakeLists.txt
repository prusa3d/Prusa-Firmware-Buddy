cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(TARGET connect)

add_executable(${TARGET})

target_sources(
  ${TARGET}
  PRIVATE ${CMAKE_SOURCE_DIR}/src/Connect/connect.cpp
          ${CMAKE_SOURCE_DIR}/src/Connect/httpc.cpp
          ${CMAKE_SOURCE_DIR}/src/Connect/httpc_data.cpp
          ${CMAKE_SOURCE_DIR}/src/Connect/os_porting.cpp
          ${CMAKE_SOURCE_DIR}/src/Connect/socket.cpp
          main.cpp
          mock/core_interface.cpp
  )

target_include_directories(
  ${TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/mock
                    ${CMAKE_SOURCE_DIR}/src/Connect/
  )

target_compile_options(${TARGET} PRIVATE -g -pthread -O0)
target_link_options(${TARGET} PRIVATE -g -O0)

target_link_libraries(${TARGET} PRIVATE pthread)

include(dev-tests.cmake)
