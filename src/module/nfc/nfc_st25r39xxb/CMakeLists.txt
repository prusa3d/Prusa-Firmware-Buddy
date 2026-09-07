add_library(st25r39xxb STATIC hw_interface.cpp ST25R39XXB.cpp)

target_include_directories(st25r39xxb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  st25r39xxb
  PRIVATE raii SG14 buddy_utils
  PUBLIC nfcv bsod
  )
