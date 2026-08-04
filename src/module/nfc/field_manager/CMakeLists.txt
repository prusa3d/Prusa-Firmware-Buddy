# RF field arbitration on top of the nfcv reader/writer interface

add_library(nfcv_field_manager OBJECT field_manager.cpp)

target_include_directories(nfcv_field_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  nfcv_field_manager
  PUBLIC nfcv
  PRIVATE log_interface
  )
