add_library(rtt_metrics_task STATIC rtt_metrics_task.cpp)

target_include_directories(rtt_metrics_task PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(rtt_metrics_task PUBLIC freertos rtt_metrics_segger)
target_link_libraries(rtt_metrics_task PRIVATE Marlin)
