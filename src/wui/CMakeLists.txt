if(NOT TARGET WUI_Config)
  message(FATAL_ERROR "Target WUI_Config does not exist.")
endif()

add_library(
  WUI
  wui_api.c
  wui.c
  http/altcp_proxyconnect.c
  http/fs.c
  http/fsdata.c
  http/httpd.c
  resources/fsdata_custom.c
)

target_include_directories(
  WUI
  PUBLIC
    /
    http
    resources
)

target_link_libraries(WUI PUBLIC WUI_Config)
target_link_libraries(
  WUI
  PRIVATE LwIP
          jsmn::jsmn
  )
