add_library(
  freertos OBJECT
  binary_semaphore.cpp
  counting_semaphore.cpp
  critical_section.cpp
  mutex.cpp
  queue.cpp
  timing.cpp
  system_tasks.cpp
  wait_condition.cpp
  )

target_link_libraries(freertos PRIVATE FreeRTOS::FreeRTOS)

target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# FIXME: Find a better place for this dispatch.
if(BOARD STREQUAL "XBUDDY_EXTENSION")
  target_compile_definitions(freertos PUBLIC FREERTOS_INTERNAL_STORAGE_SIZE=72)
else()
  target_compile_definitions(freertos PUBLIC FREERTOS_INTERNAL_STORAGE_SIZE=80)
endif()
