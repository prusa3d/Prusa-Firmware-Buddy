add_library(
  wsapi
  connect.cpp
  connection.cpp
  http_states.c
  lwsapi.cpp  
  )

target_include_directories(
  wsapi
  PUBLIC /
         ${CMAKE_SOURCE_DIR}/src/wui
         ${CMAKE_SOURCE_DIR}/src/wui/resources
         ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/src/guiapi/include
   )

target_link_libraries(wsapi PRIVATE LwIP_212)