add_library(
  freertos OBJECT binary_semaphore.cpp mutex.cpp queue.cpp wait_condition.cpp critical_section.cpp
  )

target_link_libraries(freertos PRIVATE FreeRTOS::FreeRTOS)

target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
