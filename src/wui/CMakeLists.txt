if(NOT TARGET WUI_Config)
  message(FATAL_ERROR "Target WUI_Config does not exist.")
endif()

add_library(
  WUI
  wui.c
  wui_api.c
  wui_helper_funcs.c
  http/altcp_proxyconnect.c
  http/fs.c
  http/http_client.c
  http/httpd.c
  resources/fsdata_custom.c
  progress_data_wrapper.cpp
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
          Marlin
  )
