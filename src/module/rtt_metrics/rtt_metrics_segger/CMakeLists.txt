add_library(rtt_metrics_segger STATIC peripheries_metrics.cpp rtt_metrics_segger.cpp)

target_include_directories(rtt_metrics_segger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
  rtt_metrics_segger
  PUBLIC Segger
         cobs
         rtt_metrics_structs
         rtt_metrics_serialization
         accelerometer
         byte_utils
         buddy_utils
  )
