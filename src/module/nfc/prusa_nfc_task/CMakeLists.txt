add_library(prusa_nfc_task STATIC)
target_sources(prusa_nfc_task PRIVATE prusa_nfc/nfc_task.cpp)
target_include_directories(prusa_nfc_task PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  prusa_nfc_task
  PRIVATE openprinttag
          freertos
          o1heap
          SG14
          buddy_utils
          dsdl_uavcan
          dsdl_prusa3d
          log_interface
  )
