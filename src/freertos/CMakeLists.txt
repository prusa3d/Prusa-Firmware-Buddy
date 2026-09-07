add_library(
  freertos OBJECT
  binary_semaphore.cpp
  chrono.cpp
  counting_semaphore.cpp
  critical_section.cpp
  mutex.cpp
  queue.cpp
  timing.cpp
  system_tasks.cpp
  wait_condition.cpp
  task.cpp
  )

target_link_libraries(freertos PRIVATE FreeRTOS::FreeRTOS)
target_link_libraries(freertos PUBLIC bsod byte_utils)

target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# REMOVEME BFW-6418
if(BOARD_IS_MASTER_BOARD)
  target_compile_definitions(freertos PUBLIC FREERTOS_MUTEX_POWER_PANIC_MODE=1)
else()
  target_compile_definitions(freertos PUBLIC FREERTOS_MUTEX_POWER_PANIC_MODE=0)
endif()

# FIXME: Find a better place for this dispatch.
if(BOARD STREQUAL "XBUDDY_EXTENSION"
   OR BOARD STREQUAL "ANFC"
   OR BOARD STREQUAL "TOOL_OFFSET_SENSOR"
   OR BOARD STREQUAL "INDX_HEAD"
   )
  target_compile_definitions(freertos PUBLIC FREERTOS_INTERNAL_STORAGE_SIZE=72)
  target_sources(freertos PRIVATE stream_buffer.cpp)
else()
  target_compile_definitions(freertos PUBLIC FREERTOS_INTERNAL_STORAGE_SIZE=80)
endif()
