add_executable(
  connect_tests ${CMAKE_CURRENT_SOURCE_DIR}/missing_functions.cpp
                ${CMAKE_SOURCE_DIR}/src/Connect/httpc.cpp http_client.cpp
  )
target_include_directories(
  connect_tests PRIVATE . ${CMAKE_SOURCE_DIR}/src/Connect ${CMAKE_SOURCE_DIR}/src/common
  )
add_definitions(-DUNITTESTS)

add_catch_test(connect_tests)
