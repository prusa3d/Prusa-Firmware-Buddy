add_library(lwesp INTERFACE)

target_sources(lwesp INTERFACE

)

target_include_directories(
  lwesp
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    src
    api
    cli
    include
    lwesp
    system
)

target_link_libraries(
  lwesp
  INTERFACE WUI
)
