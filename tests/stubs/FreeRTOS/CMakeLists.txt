add_library(freertos
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
  freertos
  PUBLIC .
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/ThirdParty/GCC/Posix/
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
  )

target_compile_options(freertos PUBLIC -fno-stack-protector)

