if(NOT TARGET FreeRTOS_Config)
  message(FATAL_ERROR "Target FreeRTOS_Config does not exist.")
endif()

if(MCU MATCHES "STM32F4")
  set(port_subdir "Source/portable/GCC/ARM_CM4F")
elseif(MCU MATCHES "STM32G0")
  set(port_subdir "Source/portable/GCC/ARM_CM0")
else()
  message(FATAL_ERROR "Unknown HW platform of mcu \"${MCU}\"")
endif()

add_library(
  FreeRTOS
  ${port_subdir}/port.c
  Source/CMSIS_RTOS/cmsis_os.c
  Source/croutine.c
  Source/event_groups.c
  Source/list.c
  Source/queue.c
  Source/tasks.c
  Source/timers.c
  )

target_include_directories(FreeRTOS PUBLIC Source/include Source/CMSIS_RTOS ${port_subdir})

target_link_libraries(FreeRTOS PUBLIC FreeRTOS_Config)

add_library(FreeRTOS::FreeRTOS ALIAS FreeRTOS)
