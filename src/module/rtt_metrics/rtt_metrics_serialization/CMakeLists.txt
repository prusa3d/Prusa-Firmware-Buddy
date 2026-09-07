add_library(rtt_metrics_serialization INTERFACE)

target_include_directories(rtt_metrics_serialization INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  rtt_metrics_serialization INTERFACE Bitsery::bitsery accelerometer rtt_metrics_structs
  )
