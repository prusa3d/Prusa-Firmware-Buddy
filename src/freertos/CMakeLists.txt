add_library(
  freertos OBJECT
  binary_semaphore.cpp
  counting_semaphore.cpp
  critical_section.cpp
  mutex.cpp
  queue.cpp
  delay.cpp
  system_tasks.cpp
  wait_condition.cpp
  )

target_link_libraries(freertos PRIVATE FreeRTOS::FreeRTOS)

target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
