if(NOT TARGET FreeRTOS_Config)
  message(FATAL_ERROR "Target FreeRTOS_Config does not exist.")
endif()

add_library(
  FreeRTOS
  Source/CMSIS_RTOS/cmsis_os.c
  Source/croutine.c
  Source/event_groups.c
  Source/list.c
  Source/portable/GCC/ARM_CM4F/port.c
  Source/queue.c
  Source/tasks.c
  Source/timers.c
  )

target_include_directories(
  FreeRTOS PUBLIC Source/include Source/CMSIS_RTOS Source/portable/GCC/ARM_CM4F
  )

target_link_libraries(FreeRTOS PUBLIC CMSIS FreeRTOS_Config)

add_library(FreeRTOS::FreeRTOS ALIAS FreeRTOS)
