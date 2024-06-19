add_library(freertos_stubs
  ${CMAKE_CURRENT_SOURCE_DIR}/freertos.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/croutine.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/event_groups.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/list.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/queue.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/stream_buffer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/tasks.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/timers.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/MemMang/heap_3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/ThirdParty/GCC/Posix/port.c
)

target_include_directories(
  freertos_stubs
  PUBLIC .
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/ThirdParty/GCC/Posix/
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
  )

target_compile_options(freertos_stubs PUBLIC -fno-stack-protector)

add_library(freertos_tests OBJECT
    ${CMAKE_SOURCE_DIR}/src/freertos/binary_semaphore.cpp
    ${CMAKE_SOURCE_DIR}/src/freertos/mutex.cpp
    ${CMAKE_SOURCE_DIR}/src/freertos/queue.cpp
    ${CMAKE_SOURCE_DIR}/src/freertos/wait_condition.cpp
)

# FIXME some day, this will be PRIVATE
target_link_libraries(freertos_tests PUBLIC freertos_stubs)
target_include_directories(freertos_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/freertos/include)
target_compile_definitions(freertos_tests PRIVATE -DUNITTESTS)
